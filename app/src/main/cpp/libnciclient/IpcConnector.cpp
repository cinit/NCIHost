//
// Created by kinit on 2021-10-11.
//
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <jni.h>
#include <cstdint>
#include <climits>
#include <csignal>
#include <cerrno>
#include <string>
#include <mutex>
#include <algorithm>
#include <chrono>

#include "../rpcprotocol/Log.h"
#include "../rpcprotocol/io_utils.h"

#include "IpcConnector.h"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

#define IPC_SHM_SIZE (1*1024*1024)

using IpcSocketMeta = struct {
    uint32_t pid;
    char name[108];
};

static_assert(sizeof(IpcSocketMeta) == 112, "IpcSocketMeta size error");

#define LOG_TAG "IpcConnector"

IpcConnector *volatile IpcConnector::sInstance = nullptr;

IpcConnector::IpcConnector() = default;

IpcConnector::~IpcConnector() = default;

void IpcConnector::handleLinkStart(int fd) {
    std::vector<IpcStatusListener> listeners;
    IpcProxy *obj;
    {
        std::scoped_lock<std::mutex> lk(mLock);
        if (mIpcProxy == nullptr) {
            mIpcProxy = std::make_shared<IpcProxy>();
            mIpcProxy->setName("ncihostd");
        }
        if (mNciProxy == nullptr) {
            mNciProxy = std::make_shared<NciHostDaemonProxy>();
            mNciProxy->setIpcProxy(mIpcProxy.get());
        }
        if (int err = mIpcProxy->attach(fd); err != 0) {
            LOGE("mIpcProxy.attach(%d) error: %d, %s", fd, err, strerror(err));
            return;
        }
        mIpcConnFd = fd;
        mIpcProxy->start();
        listeners = mStatusListeners;
        obj = mIpcProxy.get();
        mConnCondVar.notify_all();
    }
    LOGI("link start");
    for (auto &l: listeners) {
        l(IpcStatusEvent::IPC_CONNECTED, obj);
    }
    mIpcProxy->join();
    LOGI("link closed");
    {
        std::scoped_lock<std::mutex> lk(mLock);
        mIpcListenFd = -1;
        listeners = mStatusListeners;
    }
    if (mIpcProxy->isConnected()) {
        LOGE("mIpcProxy->join() return but mIpcProxy->isConnected() is true");
    }
    for (auto &l: listeners) {
        l(IpcStatusEvent::IPC_DISCONNECTED, nullptr);
    }
}

bool IpcConnector::handleSocketReconnect(int client) {
    LOGI("try send sockpair over fd %d", client);
    int socksFd[2] = {};
    if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, socksFd) != 0) {
        LOGE("sockpair error %s", strerror(errno));
        return false;
    }
    struct msghdr msg;
    struct iovec iov;
    union {
        struct cmsghdr cmsg;
        char cmsgSpace[CMSG_LEN(sizeof(int))];
    };
    memset(&msg, 0, sizeof(msg));
    memset(&iov, 0, sizeof(iov));
    memset(cmsgSpace, 0, sizeof(cmsgSpace));
    int c = 0;
    iov.iov_base = &c;
    iov.iov_len = 1;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = &cmsg;
    msg.msg_controllen = sizeof(cmsgSpace);
    cmsg.cmsg_len = sizeof(cmsgSpace);
    cmsg.cmsg_level = SOL_SOCKET;
    cmsg.cmsg_type = SCM_RIGHTS;
    *reinterpret_cast<int *>(CMSG_DATA(&cmsg)) = socksFd[1];
    if (sendmsg(client, &msg, 0) < 0) {
        LOGE("sendfd error %s", strerror(errno));
        close(socksFd[0]);
        close(socksFd[1]);
        close(client);
        return false;
    } else {
        int r = wait_for_input(socksFd[0], 300);
        if (r > 0) {
            char buf[24];
            ssize_t s = recv(socksFd[0], buf, 24, 0);
            if (s < 0) {
                LOGE("wait for end point send error %d %s", -r, strerror(-r));
                close(socksFd[0]);
                close(socksFd[1]);
            } else {
                close(socksFd[1]);
                close(client);
                handleLinkStart(socksFd[0]);
                return true;
            }
        } else {
            if (r < 0) {
                LOGE("wait for end point send error %d %s", -r, strerror(-r));
            } else {
                LOGE("wait for end point timeout");
            }
            close(socksFd[0]);
            close(socksFd[1]);
            close(client);
        }
    }
    return false;
}

void IpcConnector::listenForConnInternal() {
    pthread_setname_np(pthread_self(), "IpcConn");
    int listenFd = mIpcListenFd;
    while (true) {
        if (listen(listenFd, 1) < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                LOGE("listen fd error: %s", strerror(errno));
                std::scoped_lock<std::mutex> lk(mLock);
                close(listenFd);
                mIpcListenFd = -1;
                return;
            }
        } else {
            // accept
            struct sockaddr_un cliun = {};
            socklen_t socklen = sizeof(cliun);
            if (int client = accept(listenFd, (sockaddr *) (&cliun), &socklen); client >= 0) {
                ucred credentials = {};
                if (socklen_t ucred_length = sizeof(ucred);
                        ::getsockopt(client, SOL_SOCKET, SO_PEERCRED, &credentials, &ucred_length)) {
                    LOGE("getsockopt SO_PEERCRED error: %s\n", strerror(errno));
                    close(client);
                    continue;
                }
                if (credentials.uid != 0) {
                    printf("Invalid daemon UID %d, expected 0\n", credentials.uid);
                    close(client);
                    continue;
                }
                // re-establish connection here
                handleSocketReconnect(client);
            } else {
                LOGE("accept error: %s", strerror(errno));
            }
        }
    }
}

int IpcConnector::init(const char *dirPath, const char *uuidStr) {
    std::scoped_lock<std::mutex> lk(mLock);
    if (mInitialized) {
        return 0;
    }
    mIpcPidFilePath = dirPath;
    mIpcPidFilePath += "/ipc.pid";
    mIpcAbsSocketName = uuidStr;
    mIpcListenFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (mIpcListenFd == -1) {
        return errno;
    }
    struct sockaddr_un listenAddr = {0};
    listenAddr.sun_family = AF_UNIX;
    memcpy(listenAddr.sun_path + 1, mIpcAbsSocketName.c_str(), mIpcAbsSocketName.length());
    if (size_t size = offsetof(struct sockaddr_un, sun_path) + strlen(listenAddr.sun_path + 1) + 1;
            ::bind(mIpcListenFd, (struct sockaddr *) &listenAddr, (int) size) < 0) {
        int err = errno;
        LOGE("bind @%s failed, %d, %s", mIpcAbsSocketName.c_str(), err, strerror(err));
        close(mIpcListenFd);
        mIpcListenFd = -1;
        return err;
    }
    if (int err = pthread_create(&mIpcListenThread, nullptr,
                                 reinterpret_cast<void *(*)(void *)>(&IpcConnector::stpListenForConnInternal), this);
            err != 0) {
        LOGE("create thread error, %d, %s", err, strerror(err));
        close(mIpcListenFd);
        mIpcListenFd = -1;
        return err;
    }
    IpcSocketMeta meta = {};
    meta.pid = getpid();
    memcpy(meta.name, listenAddr.sun_path, std::min(108, UNIX_PATH_MAX));
    int fd = open(mIpcPidFilePath.c_str(), O_RDWR | O_CREAT, 0600);
    if (fd < 0) {
        int err = errno;
        LOGE("open gIpcPidFilePath: %s", strerror(err));
        close(mIpcListenFd);
        mIpcListenFd = -1;
        return err;
    }
    mIpcFileFlagFd = open(mIpcPidFilePath.c_str(), O_RDONLY);
    if (mIpcFileFlagFd < 0) {
        int err = errno;
        LOGE("open mIpcFileFlagFd: %s", strerror(err));
        close(mIpcListenFd);
        close(fd);
        mIpcListenFd = -1;
        return err;
    }
    if (write(fd, &meta, sizeof(IpcSocketMeta)) <= 0) {
        int err = errno;
        LOGE("write gIpcPidFilePath: %s", strerror(err));
        close(fd);
        close(mIpcListenFd);
        mIpcListenFd = -1;
        return err;
    }
    close(fd);
    mInitialized = true;
    return 0;
}

/**
 *  no regard to concurrency here, just let it go...
 */
IpcConnector &IpcConnector::getInstance() {
    if (sInstance == nullptr) {
        // sInstance will NEVER be released
        sInstance = new IpcConnector();
    }
    return *sInstance;
}

IpcProxy *IpcConnector::getIpcProxy() {
    return mIpcConnFd == -1 ? nullptr : (mIpcProxy.get());
}

INciHostDaemon *IpcConnector::getNciDaemon() {
    return mIpcProxy ? (mIpcProxy->isConnected() ? mNciProxy.get() : nullptr) : nullptr;
}

/**
 * trigger an IN_CLOSE_WRITE inotify event
 */
int IpcConnector::sendConnectRequest() {
    if (mInitialized) {
        int fd = open(mIpcPidFilePath.c_str(), O_RDWR);
        if (fd >= 0) {
            close(fd);
            return 0;
        } else {
            int err = errno;
            LOGE("sendConnectRequest: open: %s", strerror(err));
            return err;
        }
    } else {
        LOGE("sendConnectRequest but not init");
        return EBUSY;
    }
}

bool IpcConnector::isConnected() {
    return getIpcProxy() != nullptr && mIpcProxy->isConnected();
}

void IpcConnector::registerIpcStatusListener(IpcConnector::IpcStatusListener listener) {
    std::scoped_lock<std::mutex> _(mLock);
    if (std::find(mStatusListeners.begin(), mStatusListeners.end(), listener) != mStatusListeners.end()) {
        mStatusListeners.push_back(listener);
    }
}

bool IpcConnector::unregisterIpcStatusListener(IpcConnector::IpcStatusListener listener) {
    std::scoped_lock<std::mutex> _(mLock);
    mStatusListeners.erase(std::remove(mStatusListeners.begin(), mStatusListeners.end(), listener),
                           mStatusListeners.end());
    return true;
}

int IpcConnector::waitForConnection(int timeout_ms) {
    std::unique_lock lk(mLock);
    if (mIpcConnFd == -1 || mIpcProxy == nullptr) {
        return mConnCondVar.wait_for(lk, std::chrono::microseconds(timeout_ms)) == std::cv_status::timeout ? 0 : 1;
    } else {
        return 1;
    }
}
