//
// Created by kinit on 2021-05-15.
//

#include "IpcConnection.h"
#include "algorithm"
#include "mutex"
#include "unistd.h"
#include "fcntl.h"
#include "sys/stat.h"
#include "sys/socket.h"
#include <cerrno>
#include <sys/un.h>
#include "android/log.h"

using namespace std;

static mutex *gIpcNativeLock = nullptr;
pthread_t gIpcListenThread = 0;
char gIpcAbsSocketName[108];
char gIpcPidFilePath[256];
volatile int gIpcListenFd = -1;
volatile int gIpcClientConnFd = -1;
volatile int gIpcThreadErrno = -1;


extern "C" JNIEXPORT
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    gIpcNativeLock = new std::mutex;
    memset(gIpcAbsSocketName, 0, 108);
    gIpcListenThread = 0;
    gIpcClientConnFd = -1;
    gIpcListenFd = -1;
    return JNI_VERSION_1_6;
}

bool handleSocketConnected() {
    __android_log_print(ANDROID_LOG_INFO, "IPC", "connected, fd=%d", gIpcClientConnFd);
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
        if ((gIpcClientConnFd = accept(gIpcListenFd, (sockaddr *) (&cliun), &socklen)) < 0) {
            gIpcThreadErrno = errno;
        } else {
            ucred credentials = {};
            socklen_t ucred_length = sizeof(ucred);
            if (::getsockopt(gIpcClientConnFd, SOL_SOCKET, SO_PEERCRED,
                             &credentials, &ucred_length)) {
                printf("getsockopt SO_PEERCRED error: %s\n", strerror(errno));
                close(gIpcClientConnFd);
                continue;
            }
            if (credentials.uid != 0) {
                printf("Invalid daemon UID %d, expected 0\n", credentials.uid);
                close(gIpcClientConnFd);
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
        __android_log_print(ANDROID_LOG_INFO, "IPC", "started, ignored");
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, "IPC", "init ipc thread");
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
