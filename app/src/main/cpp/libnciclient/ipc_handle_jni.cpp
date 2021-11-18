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
#include "../rpcprotocol/log/Log.h"
#include "../rpcprotocol/utils/io_utils.h"

#define LOG_TAG "ipc_handle_jni"

using namespace std;
using namespace ipcprotocol;

extern "C" void __android_log_print(int level, const char *tag, const char *fmt, ...);

static void android_log_handler(Log::Level level, const char *tag, const char *msg) {
    __android_log_print(static_cast<int>(level), tag, "%s", msg);
}

template<class T>
bool jniThrowLpcResultErrorOrException(JNIEnv *env, const TypedLpcResult<T> &result) {
    if (uint32_t err = result.getError(); err != 0) {
        std::string msg;
        switch (LpcErrorCode(err)) {
            case LpcErrorCode::ERR_NO_LPC_HANDLER:
                msg = "no LPC support in the remote end";
                break;
            case LpcErrorCode::ERR_BROKEN_CONN:
                msg = "connection broken";
                break;
            case LpcErrorCode::ERR_INVALID_ARGUMENT:
                msg = "argument list mismatch";
                break;
            case LpcErrorCode::ERR_LOCAL_INTERNAL_ERROR:
                msg = "ERR_LOCAL_INTERNAL_ERROR";
                break;
            case LpcErrorCode::ERR_BAD_REQUEST:
                msg = "ERR_BAD_BUFFER";
                break;
            case LpcErrorCode::ERR_REMOTE_INTERNAL_ERROR:
                msg = "ERR_REMOTE_INTERNAL_ERROR";
                break;
            case LpcErrorCode::ERR_TIMEOUT_IN_CRITICAL_CONTEXT:
                msg = "ERR_TIMEOUT_IN_CRITICAL_CONTEXT";
                break;
            default:
                msg = std::string("unknown error: ") + std::to_string(err);
        }
        env->ThrowNew(env->FindClass("android/os/RemoteException"), msg.c_str());
        return true;
    } else if (result.hasException()) {
        if (RemoteException ex; !result.getException(&ex)) {
            env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                          "error while read exception from LpcResult");
        } else {
            env->ThrowNew(env->FindClass("java/lang/RuntimeException"), ex.what());
        }
        return true;
    }
    return false;
}

extern "C" [[maybe_unused]] JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    Log::setLogHandler(&android_log_handler);
    (void) IpcConnector::getInstance();
    return JNI_VERSION_1_6;
}

extern "C" [[maybe_unused]] JNIEXPORT void JNICALL
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


extern "C" [[maybe_unused]] JNIEXPORT jboolean JNICALL
Java_cc_ioctl_nfcncihost_daemon_IpcNativeHandler_ntPeekConnection
        (JNIEnv *env, jclass clazz) {
    IpcConnector &connector = IpcConnector::getInstance();
    return connector.isConnected();
}

extern "C" [[maybe_unused]] JNIEXPORT jboolean JNICALL
Java_cc_ioctl_nfcncihost_daemon_IpcNativeHandler_ntRequestConnection
        (JNIEnv *env, jclass) {
    IpcConnector &connector = IpcConnector::getInstance();
    if (connector.isInitialized()) {
        if (connector.isConnected()) {
            return true;
        } else {
            int errcode = connector.sendConnectRequest();
            if (errcode != 0) {
                env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                              strerror(errcode));
                return false;
            } else {
                return false;
            }
        }
    } else {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to connect before initialization");
        return false;
    }
}

extern "C" [[maybe_unused]] JNIEXPORT jboolean JNICALL
Java_cc_ioctl_nfcncihost_daemon_IpcNativeHandler_ntWaitForConnection
        (JNIEnv *env, jclass, jint timeout) {
    IpcConnector &connector = IpcConnector::getInstance();
    if (connector.isInitialized()) {
        return (jboolean) (connector.waitForConnection(timeout >= -1 ? timeout : 0) > 0);
    } else {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to connect before initialization");
        return false;
    }
}
/*
 * Class:     cc_ioctl_nfcncihost_daemon_internal_NciHostDaemonProxy
 * Method:    isConnected
 * Signature: ()Z
 */
extern "C" [[maybe_unused]] JNIEXPORT jboolean JNICALL
Java_cc_ioctl_nfcncihost_daemon_internal_NciHostDaemonProxy_isConnected
        (JNIEnv *, jobject) {
    IpcConnector &connector = IpcConnector::getInstance();
    return connector.isConnected();
}

/*
 * Class:     cc_ioctl_nfcncihost_daemon_internal_NciHostDaemonProxy
 * Method:    getVersionName
 * Signature: ()Ljava/lang/String;
 */
extern "C" [[maybe_unused]] JNIEXPORT jstring JNICALL
Java_cc_ioctl_nfcncihost_daemon_internal_NciHostDaemonProxy_getVersionName
        (JNIEnv *env, jobject) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return nullptr;
    } else {
        if (auto lpcResult = proxy->getVersionName();
                !jniThrowLpcResultErrorOrException(env, lpcResult)) {
            std::string r;
            if (lpcResult.getResult(&r)) {
                return env->NewStringUTF(r.c_str());
            } else {
                env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                              "error while read data from LpcResult");
                return nullptr;
            }
        }
        return nullptr;
    }
}

/*
 * Class:     cc_ioctl_nfcncihost_daemon_internal_NciHostDaemonProxy
 * Method:    getVersionCode
 * Signature: ()I
 */
extern "C" [[maybe_unused]] JNIEXPORT jint JNICALL
Java_cc_ioctl_nfcncihost_daemon_internal_NciHostDaemonProxy_getVersionCode
        (JNIEnv *env, jobject) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return 0;
    } else {
        if (auto lpcResult = proxy->getVersionCode();
                !jniThrowLpcResultErrorOrException(env, lpcResult)) {
            int r;
            if (lpcResult.getResult(&r)) {
                return r;
            } else {
                env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                              "error while read data from LpcResult");
                return 0;
            }
        }
        return 0;
    }
}

/*
 * Class:     cc_ioctl_nfcncihost_daemon_internal_NciHostDaemonProxy
 * Method:    getBuildUuid
 * Signature: ()Ljava/lang/String;
 */
extern "C" [[maybe_unused]] JNIEXPORT jstring JNICALL
Java_cc_ioctl_nfcncihost_daemon_internal_NciHostDaemonProxy_getBuildUuid
        (JNIEnv *env, jobject) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return nullptr;
    } else {
        if (auto lpcResult = proxy->getBuildUuid();
                !jniThrowLpcResultErrorOrException(env, lpcResult)) {
            std::string r;
            if (lpcResult.getResult(&r)) {
                return env->NewStringUTF(r.c_str());
            } else {
                env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                              "error while read data from LpcResult");
                return nullptr;
            }
        }
        return nullptr;
    }
}

/*
 * Class:     cc_ioctl_nfcncihost_daemon_internal_NciHostDaemonProxy
 * Method:    exitProcess
 * Signature: ()V
 */
extern "C" [[maybe_unused]] JNIEXPORT void JNICALL
Java_cc_ioctl_nfcncihost_daemon_internal_NciHostDaemonProxy_exitProcess
        (JNIEnv *env, jobject) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
    } else {
        if (auto lpcResult = proxy->exitProcess();
                !jniThrowLpcResultErrorOrException(env, lpcResult)) {
            if (lpcResult.isSuccessful()) {
                return;
            } else {
                env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                              "error while read data from LpcResult");
            }
        }
    }
}

/*
 * Class:     cc_ioctl_nfcncihost_daemon_internal_NciHostDaemonProxy
 * Method:    testFunction
 * Signature: (I)I
 */
extern "C" [[maybe_unused]] JNIEXPORT jint JNICALL
Java_cc_ioctl_nfcncihost_daemon_internal_NciHostDaemonProxy_testFunction
        (JNIEnv *env, jobject, jint v1) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return 0;
    } else {
        if (auto lpcResult = proxy->testFunction(v1);
                !jniThrowLpcResultErrorOrException(env, lpcResult)) {
            int r;
            if (lpcResult.getResult(&r)) {
                return r;
            } else {
                env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                              "error while read data from LpcResult");
                return 0;
            }
        }
        return 0;
    }
}
