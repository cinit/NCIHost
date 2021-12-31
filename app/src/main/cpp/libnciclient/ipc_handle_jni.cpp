//
// Created by kinit on 2021-05-15.
//

#include <sys/un.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <jni.h>
#include <cerrno>
#include <mutex>
#include <vector>
#include <tuple>
#include <condition_variable>

#include "ipc_handle_jni.h"
#include "IpcConnector.h"
#include "../rpcprotocol/log/Log.h"
#include "NciClientImpl.h"
#include "rpcprotocol/INciHostDaemon.h"

#define LOG_TAG "ipc_handle_jni"

using namespace std;
using namespace ipcprotocol;

static std::mutex g_EventMutex;
static std::condition_variable g_EventWaitCondition;
static std::vector<std::tuple<halpatch::IoSyscallEvent, std::vector<uint8_t>>> g_IoEventVec;
static std::vector<int> g_RemoteDeathVec;
static bool g_isNciHostDaemonConnectedPrevious = false;

extern "C" void __android_log_print(int level, const char *tag, const char *fmt, ...);

static void android_log_handler(Log::Level level, const char *tag, const char *msg) {
    __android_log_print(static_cast<int>(level), tag, "%s", msg);
}

static inline jstring getJString(JNIEnv *env, const std::string &str) {
    return env->NewStringUTF(str.c_str());
}

static inline jstring getJStringOrNull(JNIEnv *env, const std::string &str) {
    return str.empty() ? nullptr : getJString(env, str);
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
            case LpcErrorCode::ERR_UNKNOWN_REQUEST:
                msg = "ERR_UNKNOWN_REQUEST";
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

template<class T>
bool jniThrowLpcResultErrorOrCorruption(JNIEnv *env, const TypedLpcResult<T> &result) {
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
            return true;
        }
        return false;
    }
    return false;
}

jobject createHalServiceStatusObject(JNIEnv *env, const INciHostDaemon::DaemonStatus::HalServiceStatus &status) {
    jclass clazz = env->FindClass("cc/ioctl/nfcdevicehost/ipc/daemon/INciHostDaemon$DaemonStatus$HalServiceStatus");
    jmethodID constructor = env->GetMethodID(clazz, "<init>",
                                             "(ZIILjava/lang/String;ILjava/lang/String;Ljava/lang/String;)V");
    jobject jexecPath = getJStringOrNull(env, status.halServiceExePath);
    jobject jcontext = getJStringOrNull(env, status.halServiceProcessSecurityContext);
    jobject jlabel = getJStringOrNull(env, status.halServiceExecutableSecurityLabel);
    jobject jstatus = env->NewObject(clazz, constructor, status.isHalServiceAttached, status.halServicePid,
                                     status.halServiceUid, jexecPath, status.halServiceArch, jcontext, jlabel);
    return jstatus;
}

jobject createDaemonStatusObject(JNIEnv *env, const INciHostDaemon::DaemonStatus &status) {
    jclass clazz = env->FindClass("cc/ioctl/nfcdevicehost/ipc/daemon/INciHostDaemon$DaemonStatus");
    jmethodID constructor = env->GetMethodID(clazz, "<init>",
                                             "(ILjava/lang/String;ILjava/lang/String;Lcc/ioctl/nfcdevicehost/ipc/daemon/INciHostDaemon$DaemonStatus$HalServiceStatus;Lcc/ioctl/nfcdevicehost/ipc/daemon/INciHostDaemon$DaemonStatus$HalServiceStatus;)V");
    jobject jexecPath = getJStringOrNull(env, status.versionName);
    jobject jcontext = getJStringOrNull(env, status.daemonProcessSecurityContext);
    jobject jnfc = createHalServiceStatusObject(env, status.nfcHalServiceStatus);
    jobject jese = createHalServiceStatusObject(env, status.esePmServiceStatus);
    jobject jstatus = env->NewObject(clazz, constructor, status.processId, jexecPath, status.abiArch,
                                     jcontext, jnfc, jese);
    return jstatus;
}

void IpcNativeCallback_IpcStatusChangeListener(IpcConnector::IpcStatusEvent event, IpcTransactor *obj);

extern "C" [[maybe_unused]] JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    Log::setLogHandler(&android_log_handler);
    IpcConnector::getInstance().registerIpcStatusListener(&IpcNativeCallback_IpcStatusChangeListener);
    return JNI_VERSION_1_6;
}

extern "C" [[maybe_unused]] JNIEXPORT void JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_IpcNativeHandler_ntInitForSocketDir
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
    jclass cUtils = env->FindClass("cc/ioctl/nfcdevicehost/util/Utils");
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
Java_cc_ioctl_nfcdevicehost_ipc_daemon_IpcNativeHandler_ntPeekConnection
        (JNIEnv *env, jclass clazz) {
    IpcConnector &connector = IpcConnector::getInstance();
    return connector.isConnected();
}

extern "C" [[maybe_unused]] JNIEXPORT jboolean JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_IpcNativeHandler_ntRequestConnection
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
Java_cc_ioctl_nfcdevicehost_ipc_daemon_IpcNativeHandler_ntWaitForConnection
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
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    isConnected
 * Signature: ()Z
 */
extern "C" [[maybe_unused]] JNIEXPORT jboolean JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_isConnected
        (JNIEnv *, jobject) {
    IpcConnector &connector = IpcConnector::getInstance();
    return connector.isConnected();
}

/*
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    getVersionName
 * Signature: ()Ljava/lang/String;
 */
extern "C" [[maybe_unused]] JNIEXPORT jstring JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_getVersionName
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
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    getVersionCode
 * Signature: ()I
 */
extern "C" [[maybe_unused]] JNIEXPORT jint JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_getVersionCode
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
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    getBuildUuid
 * Signature: ()Ljava/lang/String;
 */
extern "C" [[maybe_unused]] JNIEXPORT jstring JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_getBuildUuid
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
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    getSelfPid
 * Signature: ()I
 */
extern "C" [[maybe_unused]] JNIEXPORT jint JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_getSelfPid
        (JNIEnv *env, jobject) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return 0;
    } else {
        if (auto lpcResult = proxy->getSelfPid();
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
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    exitProcess
 * Signature: ()V
 */
extern "C" [[maybe_unused]] JNIEXPORT void JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_exitProcess
        (JNIEnv *env, jobject) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
    } else {
        if (auto lpcResult = proxy->exitProcess();
                !jniThrowLpcResultErrorOrException(env, lpcResult)) {
            if (lpcResult.isTransacted()) {
                return;
            } else {
                env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                              "error while read data from LpcResult");
            }
        }
    }
}

/*
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    isDeviceSupported
 * Signature: ()Z
 */
extern "C" [[maybe_unused]] JNIEXPORT jboolean JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_isDeviceSupported
        (JNIEnv *env, jobject) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return 0;
    } else {
        if (auto lpcResult = proxy->isDeviceSupported();
                !jniThrowLpcResultErrorOrException(env, lpcResult)) {
            bool r;
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
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    isHwServiceConnected
 * Signature: ()Z
 */
extern "C" [[maybe_unused]] JNIEXPORT jboolean JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_isHwServiceConnected
        (JNIEnv *env, jobject) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return 0;
    } else {
        if (auto lpcResult = proxy->isHwServiceConnected();
                !jniThrowLpcResultErrorOrException(env, lpcResult)) {
            bool r;
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
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    initHwServiceConnection
 * Signature: ([Ljava/lang/String;)Z
 */
extern "C" [[maybe_unused]] JNIEXPORT jboolean JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_initHwServiceConnection
        (JNIEnv *env, jobject, jobjectArray jstrSoPathArray) {
    if (jstrSoPathArray == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/NullPointerException"),
                      "null so path array");
        return 0;
    }
    std::string path1, path2;
    // length of the array should be 2
    if (env->GetArrayLength(jstrSoPathArray) != 2) {
        env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"),
                      "invalid so path array length, expected 2");
        return 0;
    }
    auto jstr1 = static_cast<jstring>(env->GetObjectArrayElement(jstrSoPathArray, 0));
    if (jstr1 == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/NullPointerException"),
                      "null jstrSoPathArray[0]");
        return 0;
    }
    auto jstr2 = static_cast<jstring>(env->GetObjectArrayElement(jstrSoPathArray, 1));
    if (jstr2 == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/NullPointerException"),
                      "null jstrSoPathArray[1]");
        return 0;
    }
    const char *strSoPath1 = env->GetStringUTFChars(jstr1, nullptr);
    if (strSoPath1 == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/OutOfMemoryError"),
                      "out of memory");
        return 0;
    }
    path1 = strSoPath1;
    env->ReleaseStringUTFChars(jstr1, strSoPath1);
    const char *strSoPath2 = env->GetStringUTFChars(jstr2, nullptr);
    if (strSoPath2 == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/OutOfMemoryError"),
                      "out of memory");
        return 0;
    }
    path2 = strSoPath2;
    env->ReleaseStringUTFChars(jstr2, strSoPath2);
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return false;
    } else {
        if (auto lpcResult = proxy->initHwServiceConnection({path1, path2});
                !jniThrowLpcResultErrorOrCorruption(env, lpcResult)) {
            bool r;
            if (lpcResult.hasException()) {
                RemoteException exception = {1, EBADMSG, "invalid exception data structure"};
                (void) lpcResult.getException(&exception);
                env->ThrowNew(env->FindClass("java/io/IOException"), exception.what());
                return false;
            }
            if (lpcResult.getResult(&r)) {
                return r;
            } else {
                env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                              "error while read data from LpcResult");
                return false;
            }
        }
        return false;
    }
}


void NciClientImpl_forwardRemoteIoEvent(const halpatch::IoSyscallEvent &event, const std::vector<uint8_t> &payload) {
    std::scoped_lock<std::mutex> lock(g_EventMutex);
    g_IoEventVec.emplace_back(event, payload);
    g_EventWaitCondition.notify_all();
}

void NciClientImpl_forwardRemoteDeathEvent(int pid) {
    std::scoped_lock<std::mutex> lock(g_EventMutex);
    g_RemoteDeathVec.emplace_back(pid);
    g_EventWaitCondition.notify_all();
}

void IpcNativeCallback_IpcStatusChangeListener(IpcConnector::IpcStatusEvent event, IpcTransactor *obj) {
    std::unique_lock lock(g_EventMutex);
    // don't change g_isNciHostDaemonConnectedPrevious here, this value is a 'dirty' detection
    if (event == IpcConnector::IpcStatusEvent::IPC_CONNECTED) {
        if (!g_isNciHostDaemonConnectedPrevious) {
            g_EventWaitCondition.notify_all();
        }
    } else if (event == IpcConnector::IpcStatusEvent::IPC_DISCONNECTED) {
        if (g_isNciHostDaemonConnectedPrevious) {
            g_EventWaitCondition.notify_all();
        }
    }
}

/*
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    waitForEvent
 * Signature: ()Lcc/ioctl/nfcdevicehost/ipc/daemon/internal/NciHostDaemonProxy/NativeEventPacket;
 */
extern "C" [[maybe_unused]] JNIEXPORT jobject JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_waitForEvent
        (JNIEnv *env, jobject) {
    std::unique_lock<std::mutex> lock(g_EventMutex);
    if (g_IoEventVec.empty() && g_RemoteDeathVec.empty()) {
        g_EventWaitCondition.wait(lock);
    }
    // check for connection loss
    if (bool conn = IpcConnector::getInstance().isConnected(); conn != g_isNciHostDaemonConnectedPrevious) {
        g_isNciHostDaemonConnectedPrevious = conn;
        // send a null event to notify the upper layer that the connection state has changed
        return nullptr;
    }
    // check for remote death
    if (!g_RemoteDeathVec.empty()) {
        int deathEvent = g_RemoteDeathVec.back();
        g_RemoteDeathVec.pop_back();
        jmethodID ctor = env->GetMethodID(
                env->FindClass("cc/ioctl/nfcdevicehost/ipc/daemon/INciHostDaemon$RemoteDeathPacket"),
                "<init>", "(I)V");
        jobject packet = env->NewObject(
                env->FindClass("cc/ioctl/nfcdevicehost/ipc/daemon/INciHostDaemon$RemoteDeathPacket"),
                ctor, deathEvent);
        return packet;
    }
    // check for io event
    if (!g_IoEventVec.empty()) {
        halpatch::IoSyscallEvent event = std::get<0>(g_IoEventVec.back());
        std::vector<uint8_t> payload = std::get<1>(g_IoEventVec.back());
        g_IoEventVec.pop_back();
        jmethodID ctor = env->GetMethodID(
                env->FindClass("cc/ioctl/nfcdevicehost/ipc/daemon/internal/NciHostDaemonProxy$RawIoEventPacket"),
                "<init>", "([B[B)V");
        jbyteArray buffer1 = env->NewByteArray(sizeof(halpatch::IoSyscallEvent));
        env->SetByteArrayRegion(buffer1, 0, sizeof(halpatch::IoSyscallEvent), (jbyte *) &event);
        jbyteArray buffer2 = nullptr;
        if (!payload.empty()) {
            buffer2 = env->NewByteArray(payload.size());
            env->SetByteArrayRegion(buffer2, 0, payload.size(), (jbyte *) payload.data());
        }
        jobject packet = env->NewObject(
                env->FindClass("cc/ioctl/nfcdevicehost/ipc/daemon/internal/NciHostDaemonProxy$RawIoEventPacket"),
                ctor, buffer1, buffer2);
        return packet;
    }
    // should not happen
    env->ThrowNew(env->FindClass("java/io/IOException"), "internal error");
    return nullptr;
}

/*
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_IpcNativeHandler
 * Method:    getKernelArchitecture
 * Signature: ()Ljava/lang/String;
 */
extern "C" [[maybe_unused]] JNIEXPORT jstring JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_IpcNativeHandler_getKernelArchitecture
        (JNIEnv *env, jclass) {
    struct utsname uts = {};
    if (uname(&uts) != 0) {
        int err = errno;
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                      ("uname() failed: " + std::string(strerror(err))).c_str());
        return nullptr;
    }
    return env->NewStringUTF(uts.machine);
}

/*
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_IpcNativeHandler
 * Method:    getIpcPidFileFD
 * Signature: ()I
 */
extern "C" [[maybe_unused]] JNIEXPORT jint JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_IpcNativeHandler_getIpcPidFileFD
        (JNIEnv *, jclass) {
    const IpcConnector &connector = IpcConnector::getInstance();
    return int(connector.getIpcFileFlagFd());
}

/*
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    ntGetHistoryIoEvents
 * Signature: (II)Lcc/ioctl/nfcdevicehost/ipc/daemon/internal/NciHostDaemonProxy/RawHistoryIoEventList;
 */
extern "C" [[maybe_unused]] JNIEXPORT jobject JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_ntGetHistoryIoEvents
        (JNIEnv *env, jobject, jint reqStartIndex, jint reqCount) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return nullptr;
    } else {
        if (auto lpcResult = proxy->getHistoryIoOperations(uint32_t(reqStartIndex), uint32_t(reqCount));
                !jniThrowLpcResultErrorOrException(env, lpcResult)) {
            INciHostDaemon::HistoryIoOperationEventList r;
            if (lpcResult.getResult(&r)) {
                uint32_t totalStart = r.totalStartIndex;
                uint32_t totalCount = r.totalCount;
                jclass clRawEventList = env->FindClass(
                        "cc/ioctl/nfcdevicehost/ipc/daemon/internal/NciHostDaemonProxy$RawHistoryIoEventList");
                jclass clRawEvent = env->FindClass(
                        "cc/ioctl/nfcdevicehost/ipc/daemon/internal/NciHostDaemonProxy$RawIoEventPacket");
                jmethodID ctorEvent = env->GetMethodID(clRawEvent, "<init>", "([B[B)V");
                jmethodID ctorList = env->GetMethodID(clRawEventList, "<init>",
                                                      "(II[Lcc/ioctl/nfcdevicehost/ipc/daemon/internal/NciHostDaemonProxy$RawIoEventPacket;)V");
                size_t resultSize = r.events.size();
                jobjectArray array = env->NewObjectArray(resultSize, clRawEvent, nullptr);
                for (size_t i = 0; i < resultSize; ++i) {
                    jbyteArray buffer1 = env->NewByteArray(sizeof(halpatch::IoSyscallEvent));
                    env->SetByteArrayRegion(buffer1, 0, sizeof(halpatch::IoSyscallEvent),
                                            (jbyte *) &r.events[i]);
                    jbyteArray buffer2 = nullptr;
                    if (!r.payloads[i].empty()) {
                        buffer2 = env->NewByteArray(r.payloads[i].size());
                        env->SetByteArrayRegion(buffer2, 0, r.payloads[i].size(),
                                                (jbyte *) r.payloads[i].data());
                    }
                    jobject event = env->NewObject(clRawEvent, ctorEvent, buffer1, buffer2);
                    env->SetObjectArrayElement(array, i, event);
                }
                return env->NewObject(clRawEventList, ctorList, jint(totalStart), jint(totalCount), array);
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
* Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
* Method:    clearHistoryIoEvents
* Signature: ()Z
*/
extern "C" [[maybe_unused]] JNIEXPORT jboolean JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_clearHistoryIoEvents
        (JNIEnv *env, jobject) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return 0;
    } else {
        if (auto lpcResult = proxy->clearHistoryIoEvents();
                !jniThrowLpcResultErrorOrException(env, lpcResult)) {
            bool r;
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
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    getDaemonStatus
 * Signature: ()Lcc/ioctl/nfcdevicehost/ipc/daemon/INciHostDaemon/DaemonStatus;
 */
extern "C" [[maybe_unused]] JNIEXPORT jobject JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_getDaemonStatus
        (JNIEnv *env, jobject) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return nullptr;
    } else {
        if (auto lpcResult = proxy->getDaemonStatus();
                !jniThrowLpcResultErrorOrException(env, lpcResult)) {
            INciHostDaemon::DaemonStatus r;
            if (lpcResult.getResult(&r)) {
                jobject jstatus = createDaemonStatusObject(env, r);
                return jstatus;
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
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    deviceDriverWriteRaw
 * Signature: ([B)I
 */
extern "C" [[maybe_unused]] JNIEXPORT  jint JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_deviceDriverWriteRaw
        (JNIEnv *env, jobject, jbyteArray jdata) {
    if (jdata == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/NullPointerException"), "data is null");
        return 0;
    }
    std::vector<uint8_t> data;
    jsize len = env->GetArrayLength(jdata);
    data.resize(len);
    env->GetByteArrayRegion(jdata, 0, len, reinterpret_cast<jbyte *>(data.data()));
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return 0;
    } else {
        if (auto lpcResult = proxy->deviceDriverWriteRawBuffer(data);
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
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    deviceDriverIoctl0
 * Signature: (IJ)I
 */
extern "C" [[maybe_unused]] JNIEXPORT jint JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_deviceDriverIoctl0
        (JNIEnv *env, jobject, jint request, jlong arg) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return 0;
    } else {
        if (auto lpcResult = proxy->deviceDriverIoctl0(uint32_t(request), uint64_t(arg));
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
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    isAndroidNfcServiceConnected
 * Signature: ()Z
 */
extern "C" [[maybe_unused]] JNIEXPORT jboolean JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_isAndroidNfcServiceConnected
        (JNIEnv *env, jobject) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return 0;
    } else {
        if (auto lpcResult = proxy->isAndroidNfcServiceConnected();
                !jniThrowLpcResultErrorOrException(env, lpcResult)) {
            bool r;
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
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    connectToAndroidNfcService
 * Signature: ()Z
 */
extern "C" [[maybe_unused]] JNIEXPORT jboolean JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_connectToAndroidNfcService
        (JNIEnv *env, jobject) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return 0;
    } else {
        if (auto lpcResult = proxy->connectToAndroidNfcService();
                !jniThrowLpcResultErrorOrException(env, lpcResult)) {
            bool r;
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
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    isNfcDiscoverySoundDisabled
 * Signature: ()Z
 */
extern "C" [[maybe_unused]] JNIEXPORT jboolean JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_isNfcDiscoverySoundDisabled
        (JNIEnv *env, jobject) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return 0;
    } else {
        if (auto lpcResult = proxy->isNfcDiscoverySoundDisabled();
                !jniThrowLpcResultErrorOrException(env, lpcResult)) {
            bool r;
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
 * Class:     cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy
 * Method:    setNfcDiscoverySoundDisabled
 * Signature: (Z)Z
 */
extern "C" [[maybe_unused]] JNIEXPORT jboolean JNICALL
Java_cc_ioctl_nfcdevicehost_ipc_daemon_internal_NciHostDaemonProxy_setNfcDiscoverySoundDisabled
        (JNIEnv *env, jobject, jboolean jdisable) {
    IpcConnector &connector = IpcConnector::getInstance();
    INciHostDaemon *proxy = connector.getNciDaemon();
    if (proxy == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "attempt to transact while proxy object not available");
        return 0;
    } else {
        if (auto lpcResult = proxy->setNfcDiscoverySoundDisabled(jdisable);
                !jniThrowLpcResultErrorOrException(env, lpcResult)) {
            bool r;
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
