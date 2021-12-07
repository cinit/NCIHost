//
// Created by kinit on 2021-05-14.
//

#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <linux/limits.h>

#include <cstdio>
#include <cerrno>
#include <cstddef>
#include <algorithm>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <memory>
#include <csignal>

#include "common.h"
#include "daemon.h"
#include "../ipc/IpcStateController.h"
#include "rpcprotocol/log/Log.h"
#include "rpcprotocol/protocol/rpc_struct.h"
#include "rpcprotocol/protocol/IpcTransactor.h"

#define LOG_TAG "ncihostd"

using namespace std;
using namespace ipcprotocol;

typedef struct {
    uint32_t pid;
    char name[108];
} IpcSocketMeta;

static_assert(sizeof(IpcSocketMeta) == 112, "IpcSocketMeta size error");

void startRssWatchdog();

bool waitForInotifyWriteEvent(const char *path) {
    // TODO: handle event if file deleted
    char inotifyBuffer[sizeof(struct inotify_event) + NAME_MAX + 1] = {};
    inotify_event &inotifyEvent = *((inotify_event *) &inotifyBuffer);
    int inotifyFd = inotify_init1(O_CLOEXEC);
    if (inotifyFd < 0) {
        LOGE("inotify_init1: %d, %s", errno, strerror(errno));
        return false;
    }
    if (inotify_add_watch(inotifyFd, path, IN_MODIFY | IN_CLOSE_WRITE | IN_MOVE | IN_DELETE_SELF | IN_DELETE) < 0) {
        LOGE("inotify_add_watch: %d, %s", errno, strerror(errno));
        close(inotifyFd);
        return false;
    }
    while (true) {
        int size = read(inotifyFd, inotifyBuffer, sizeof(inotifyBuffer));
        if (size < 0) {
            if (errno != EINTR) {
                LOGE("wait for inotify error:%d, %s", errno, strerror(errno));
                close(inotifyFd);
                return false;
            } else {
                continue;
            }
        } else {
            // event received
            break;
        }
    }
    close(inotifyFd);
    return access(path, R_OK) == 0;
}

void handleLinkStart(int fd) {
    LOGI("link start, fd=%d", fd);
    IpcStateController::getInstance().attachIpcSeqPacketSocket(fd);
}

bool doConnect(const IpcSocketMeta &target, int uid) {
    sockaddr_un targetAddr = {};
    int clientFd;
    {
        char buf[108];
        snprintf(buf, 108, "/proc/%d/stat", target.pid);
        if (access(buf, F_OK) != 0) {
            LOGW("process %d does not exists", target.pid);
            return false;
        }
    }
    if ((clientFd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("client socket error");
        return false;
    }
    targetAddr.sun_family = AF_UNIX;
    memcpy(targetAddr.sun_path, target.name, 108);
    size_t size = offsetof(struct sockaddr_un, sun_path) + strlen(targetAddr.sun_path + 1) + 1;
    if (::connect(clientFd, (struct sockaddr *) &targetAddr, size) < 0) {
        perror("connect error");
        close(clientFd);
        return false;
    }
    ucred credentials = {};
    socklen_t ucred_length = sizeof(ucred);
    if (::getsockopt(clientFd, SOL_SOCKET, SO_PEERCRED, &credentials, &ucred_length) < 0) {
        LOGE("getsockopt SO_PEERCRED error: %s", strerror(errno));
        close(clientFd);
        return false;
    }
    if (credentials.uid != uid) {
        LOGE("Invalid UID %d ,expected %d", credentials.uid, uid);
        close(clientFd);
        return false;
    }
    LOGI("Connecting...");
    // recv from cmsg SCM_RIGHTS
    char cmptrBuf[CMSG_LEN(sizeof(int))];
    cmsghdr *cmsg = (cmsghdr *) cmptrBuf;
    char dummy[1]; // the max buf in msg.
    iovec iov[1];
    iov[0].iov_base = dummy;
    iov[0].iov_len = 1;
    msghdr msg;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    msg.msg_control = cmsg;
    msg.msg_controllen = CMSG_LEN(sizeof(int));
    ssize_t ret = recvmsg(clientFd, &msg, 0);
    if (ret == -1) {
        LOGE("SCM_RIGHTS recvmsg error: %d: %s", errno, strerror(errno));
        return false;
    }
    int targetFd = *(int *) CMSG_DATA(cmsg);
    EventTransactionHeader h = {{sizeof(EventTransactionHeader), TrxnType::TRXN_TYPE_EVENT},
                                0, 0, 0, 0};
    if (send(targetFd, &h, sizeof(EventTransactionHeader), 0) < 0) {
        LOGE("send test msg on SCM_RIGHTS fd error: %d: %s", errno, strerror(errno));
        close(clientFd);
        close(targetFd);
        return false;
    }
    close(clientFd);
    handleLinkStart(targetFd);
    LOGI("disconnected");
    return true;
}


timespec gStartTime;

void log_print_handler(Log::Level level, const char *tag, const char *msg) {
    timespec systime;
    clock_gettime(CLOCK_MONOTONIC, &systime);
    uint64_t dsec = systime.tv_sec - gStartTime.tv_sec;
    double reltime = double(dsec) + double(int64_t(systime.tv_nsec) - int64_t(gStartTime.tv_sec)) / 1000000000.0;
    char buf[64];
    snprintf(buf, 64, "%.06f", reltime);
    std::cout << buf << " " << (int) level << " " << tag << " " << msg << std::endl;
    std::cout.flush();
}

int startup_do_daemonize() {
    // set cwd and umask
    if (chdir("/") != 0) {
        printf("chdir error: %d, %s", errno, strerror(errno));
        return -1;
    }
    umask(0);
    // ignore signal SIGHUP and SIGPIPE
    struct sigaction sa = {};
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if ((sigaction(SIGHUP, &sa, nullptr) < 0) || (sigaction(SIGPIPE, &sa, nullptr) < 0)) {
        printf("sigaction error: %d, %s", errno, strerror(errno));
        exit(-1);
    }
    // fork to set session id
    pid_t pid = fork();
    if (pid < 0) {
        printf("fork error: %d, %s", errno, strerror(errno));
        return -1;
    }
    if (pid > 0) {
        exit(0);
    }
    // change to new session
    if (setsid() < 0) {
        printf("setsid error: %d, %s", errno, strerror(errno));
        return -1;
    }
    // fork again to drop tty
    pid = fork();
    if (pid < 0) {
        printf("fork error: %d, %s", errno, strerror(errno));
        return -1;
    }
    if (pid > 0) {
        exit(0);
    }
    // close stdin, stdout, stderr
    int fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
        printf("open /dev/null error: %d, %s", errno, strerror(errno));
        return -1;
    }
    for (int i = 0; i < 3; i++) {
        dup2(fd, i);
    }
    close(fd);
    return 0;
}

extern "C" void startDaemon(int uid, const char *ipcFilePath, int daemonize) {
    clock_gettime(CLOCK_MONOTONIC, &gStartTime);
    if (daemonize) {
        // daemonize process
        int err = startup_do_daemonize();
        if (err < 0) {
            printf("startup_do_daemonize error: %d", err);
            exit(-1);
        }
    } else {
        // do not daemonize, just use stdout as log output
        Log::setLogHandler(&log_print_handler);
    }
    if (std::string(ipcFilePath).find("/ipc.pid") == std::string::npos) {
        LOGE("INVALID PATH: %s", ipcFilePath);
        return;
    }
    startRssWatchdog();
    while (true) {
        int ipcFileFd = open(ipcFilePath, O_RDONLY);
        if (ipcFileFd < 0) {
            if (errno == EINTR || errno == ENOENT) {
                // ignore, go to wait for inotify
            } else {
                LOGE("open %s failed, %d, %s", ipcFilePath, errno, strerror(errno));
                _exit(1);
            }
        } else {
            // read UDS socket name
            lseek(ipcFileFd, 0, SEEK_SET);
            IpcSocketMeta meta = {};
            constexpr int ipcStructLen = sizeof(IpcSocketMeta);
            ssize_t len = 0;
            do {
                ssize_t i = read(ipcFileFd, &meta, ipcStructLen - len);
                if (i < 0) {
                    break;
                }
                len += i;
            } while (len < ipcStructLen);
            if (len < ipcStructLen) {
                LOGE("read IpcSocketMeta failed: %s", strerror(errno));
                // retry
            } else {
                // do connect
                LOGD("try connect: pid=%d, name=@%s", meta.pid, meta.name + 1);
                if (doConnect(meta, uid)) {
                    LOGI("loop disconnect from pid=%d, name=@%s", meta.pid, meta.name + 1);
                } else {
                    LOGE("connect failed, do loop");
                }
            }
            close(ipcFileFd);
        }
        // wait inotify
        if (!waitForInotifyWriteEvent(ipcFilePath)) {
            LOGE("ipc inotify listen failed... quit.");
            _exit(1);
        }
        LOGV("try reconnect");
        usleep(300000);
    }
}


static void *rss_watchdog_procedure(void *) {
    int lastRss = 0;
    while (true) {
        int fd;
        char buf[256] = {0};
        fd = open("/proc/self/status", O_RDONLY);
        SharedBuffer buffer;
        ssize_t i;
        ssize_t current = 0;
        while ((i = read(fd, buf, 128)) > 0) {
            buffer.ensureCapacity(current + i);
            memcpy(buffer.at<char>(current), buf, i);
            current += i;
        }
        close(fd);
        std::string status = {reinterpret_cast<char *>( buffer.get()), (size_t) current};
        int startpos = (int) status.find("VmRSS:") + (int) strlen("VmRSS:");
        int endpos = (int) status.find("kB", startpos);
        memset(buf, 0, 128);
        memcpy(buf, reinterpret_cast<char *>(buffer.get()) + startpos, endpos - startpos);
        int rss_kb = 0;
        sscanf(buf, "%d", &rss_kb);
        if (rss_kb == 0) {
            LOGE("error read VmRSS");
        } else {
            if (std::abs(lastRss - rss_kb) > 1024) {
                LOGD("rss = %dK", rss_kb);
                lastRss = rss_kb;
            }
            if (rss_kb > 64 * 1024) {
                LOGE("Rss too large %dK, kill process", rss_kb);
                _exit(12);
            }
        }
        usleep(10000000);
    }
}

void startRssWatchdog() {
    pthread_t tid;
    pthread_create(&tid, nullptr, &rss_watchdog_procedure, nullptr);
}
