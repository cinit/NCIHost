//
// Created by kinit on 2021-05-15.
//

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <jni.h>

#include <climits>
#include <csignal>
#include <cerrno>
#include <mutex>
#include <algorithm>

#include "ipc_handle_jni.h"
#include "IpcConnector.h"
#include "../rpcprotocol/Log.h"
#include "../rpcprotocol/rpcprotocol.h"
#include "../rpcprotocol/io_utils.h"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

#define LOG_TAG "IPC"

using namespace std;

static mutex *gIpcNativeLock = nullptr;
pthread_t gIpcListenThread = 0;
char gIpcAbsSocketName[108];
char gIpcPidFilePath[256];
volatile int gIpcListenFd = -1;
volatile int gIpcClientHandOverConnFd = -1;
volatile int gIpcConnFd = -1;
volatile int gIpcThreadErrno = -1;

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    gIpcNativeLock = new std::mutex;
    memset(gIpcAbsSocketName, 0, 108);
    gIpcListenThread = 0;
    gIpcClientHandOverConnFd = -1;
    gIpcListenFd = -1;
    gIpcConnFd = -1;
    return JNI_VERSION_1_6;
}

bool handleSocketConnected() {
    LOGI("try send sockpair over fd %d", gIpcClientHandOverConnFd);
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
    char c = 0;
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
    if (sendmsg(gIpcClientHandOverConnFd, &msg, 0) < 0) {
        LOGE("sendfd error %s", strerror(errno));
        close(socksFd[0]);
        close(socksFd[1]);
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
                ..
            }
        } else {
            if (r < 0) {
                LOGE("wait for end point send error %d %s", -r, strerror(-r));
            } else {
                LOGE("wait for end point timeout");
            }
            close(socksFd[0]);
            close(socksFd[1]);
        }
    }
    close(gIpcConnFd);
    gIpcConnFd = -1;
    close(gIpcListenFd);
    gIpcListenFd = -1;
    // send GetVersion RPC request and wait for result blocking, timeout is 300ms

    return true;
}

void *fIpcListenProc(void *) {
    pthread_setname_np(pthread_self(), "IpcListener");
    if (listen(gIpcListenFd, 1) < 0) {
        gIpcThreadErrno = errno;
    }
    struct sockaddr_un cliun = {};
    socklen_t socklen = sizeof(cliun);
    gIpcThreadErrno = 0;
    do {
        if ((gIpcClientHandOverConnFd = accept(gIpcListenFd, (sockaddr *) (&cliun), &socklen)) < 0) {
            gIpcThreadErrno = errno;
        } else {
            ucred credentials = {};
            socklen_t ucred_length = sizeof(ucred);
            if (::getsockopt(gIpcClientHandOverConnFd, SOL_SOCKET, SO_PEERCRED,
                             &credentials, &ucred_length)) {
                printf("getsockopt SO_PEERCRED error: %s\n", strerror(errno));
                close(gIpcClientHandOverConnFd);
                continue;
            }
            if (credentials.uid != 0) {
                printf("Invalid daemon UID %d, expected 0\n", credentials.uid);
                close(gIpcClientHandOverConnFd);
                continue;
            }
            if (handleSocketConnected()) {
                break;
            }
        }
    } while (errno == 0);
    close(gIpcListenFd);
    gIpcListenFd = -1;
    gIpcListenThread = 0;
    return nullptr;
}

extern "C"
JNIEXPORT void JNICALL
Java_cc_ioctl_nfcncihost_daemon_IpcNativeHandler_ntInitForSocketDir
        (JNIEnv *env, jclass clazz, jstring name) {
    char buf[256];
    lock_guard<mutex> _(*gIpcNativeLock);
    if (gIpcListenThread != 0 && pthread_kill(gIpcListenThread, 0) == EINVAL) {
        LOGI("started, ignored");
        return;
    }
    LOGI("init ipc thread");
    int len = env->GetStringLength(name);
    if (len > 255) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), "path too long");
        return;
    }
    const char *dirPath = env->GetStringUTFChars(name, nullptr);
    if (dirPath == nullptr) {
        return;// with exception
    }
    snprintf(gIpcPidFilePath, 256, "%s/ipc.pid", dirPath);
    env->ReleaseStringUTFChars(name, dirPath);
    gIpcListenFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (gIpcListenFd < 0) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                      (snprintf(buf, 256, "socket: %s", strerror(errno)), buf));
        return;
    }
    jclass cUtils = env->FindClass("cc/ioctl/nfcncihost/util/Utils");
    jstring uuid = (jstring) env->
            CallStaticObjectMethod(cUtils, env->
            GetStaticMethodID(cUtils, "generateUuid", "()Ljava/lang/String;"));
    env->GetStringUTFRegion(uuid, 0, env->GetStringLength(uuid), gIpcAbsSocketName + 1);
    struct sockaddr_un listenAddr = {};
    listenAddr.sun_family = AF_UNIX;
    memcpy(listenAddr.sun_path, gIpcAbsSocketName, min(108, UNIX_PATH_MAX));
    int size = offsetof(struct sockaddr_un, sun_path) + strlen(listenAddr.sun_path + 1) + 1;
    if (::bind(gIpcListenFd, (struct sockaddr *) &listenAddr, size) < 0) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                      (snprintf(buf, 256, "bind: %s", strerror(errno)), buf));
        return;
    }
    gIpcThreadErrno = -1;
    pthread_create(&gIpcListenThread, nullptr, &fIpcListenProc, nullptr);
    usleep(300000);
    if (gIpcThreadErrno != 0) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                      (gIpcThreadErrno == -1) ? "thread unresponsive" :
                      (snprintf(buf, 256, "remote: %s", strerror(gIpcThreadErrno)), buf));
        return;
    }
    IpcSocketMeta meta = {};
    meta.pid = getpid();
    memcpy(meta.name, gIpcAbsSocketName, min(108, UNIX_PATH_MAX));
    int fd = open(gIpcPidFilePath, O_RDWR | O_CREAT, 0600);
    if (fd < 0) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                      (snprintf(buf, 256, "open gIpcPidFilePath: %s", strerror(errno)), buf));
        return;
    }
    if (write(fd, &meta, sizeof(IpcSocketMeta)) <= 0) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                      (snprintf(buf, 256, "write gIpcPidFilePath: %s", strerror(errno)), buf));
        return;
    }
    close(fd);
}


extern "C"
JNIEXPORT jobject JNICALL
Java_cc_ioctl_nfcncihost_daemon_IpcNativeHandler_ntPeekConnection
        (JNIEnv *env, jclass clazz) {
    // TODO: implement ntPeekConnection()
    return nullptr;
}
