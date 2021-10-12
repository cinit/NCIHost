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

#define LOG_TAG "ipc_handle_jni"

using namespace std;
extern "C" void __android_log_print(int level, const char *tag, const char *fmt, ...);

static void android_log_handler(Log::Level level, const char *tag, const char *msg) {
    __android_log_print(static_cast<int>(level), tag, "%s", msg);
}

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    Log::setLogHandler(&android_log_handler);
    IpcConnector &connector = IpcConnector::getInstance();
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL
Java_cc_ioctl_nfcncihost_daemon_IpcNativeHandler_ntInitForSocketDir
        (JNIEnv *env, jclass clazz, jstring name) {
    char buf[256];
    int len = env->GetStringLength(name);
    if (len > 108) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), "path too long");
        return;
    }
    const char *dirPath = env->GetStringUTFChars(name, nullptr);
    if (dirPath == nullptr) {
        return;// with exception
    }
    jclass cUtils = env->FindClass("cc/ioctl/nfcncihost/util/Utils");
    jstring uuid = (jstring) env->
            CallStaticObjectMethod(cUtils, env->
            GetStaticMethodID(cUtils, "generateUuid", "()Ljava/lang/String;"));
    char uuidStr[64] = {};
    env->GetStringUTFRegion(uuid, 0, env->GetStringLength(uuid), uuidStr);
    IpcConnector &connector = IpcConnector::getInstance();
    int err = connector.init(dirPath, uuidStr);
    env->ReleaseStringUTFChars(name, dirPath);
    if (err != 0) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                      (std::string("error init ipc handle: errno: ")
                       + std::to_string(err) + ", " + strerror(err)).c_str());
    }
}


extern "C"
JNIEXPORT jobject JNICALL
Java_cc_ioctl_nfcncihost_daemon_IpcNativeHandler_ntPeekConnection
        (JNIEnv *env, jclass clazz) {
    // TODO: implement ntPeekConnection()
    return nullptr;
}
