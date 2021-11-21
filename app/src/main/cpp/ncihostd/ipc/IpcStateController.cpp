//
// Created by kinit on 2021-10-12.
//

#include "rpcprotocol/log/Log.h"

#include "IpcStateController.h"

#define LOG_TAG "ncihostd"

using namespace ipcprotocol;

void IpcStateController::attachIpcSeqPacketSocket(int fd) {
    if (int err = mIpcProxy.attach(fd); err != 0) {
        LOGE("IpcProxy attach fd %d error %s", fd, strerror(err));
        close(fd);
    }
    mIpcProxy.start();
    mIpcProxy.join();
}

IpcStateController &IpcStateController::getInstance() {
    static IpcStateController *volatile sInstance = nullptr;
    if (sInstance == nullptr) {
        // sInstance will NEVER be released
        auto *p = new IpcStateController();
        p->init();
        sInstance = p;
    }
    return *sInstance;
}

void IpcStateController::init() {
    mIpcProxy.setFunctionHandler(&IpcStateController::cbLpcHandler);
    mIpcProxy.setEventHandler(&IpcStateController::cbEventHandler);
}

bool IpcStateController::cbLpcHandler(const IpcProxy::LpcEnv &env, LpcResult &result,
                                      uint32_t funcId, const ArgList &args) {
    return IpcStateController::getInstance().dispatchLpcRequest(env, result, funcId, args);
}

void IpcStateController::cbEventHandler(const IpcProxy::LpcEnv &env, uint32_t funcId, const ArgList &args) {
    LOGI("ignore event %x", funcId);
}

bool IpcStateController::dispatchLpcRequest(const IpcProxy::LpcEnv &env, LpcResult &result, uint32_t funcId,
                                            const ArgList &args) {
    return mDaemon.dispatchLpcInvocation(env, result, funcId, args);
}

NciHostDaemonImpl &IpcStateController::getNciHostDaemon() {
    return mDaemon;
}

IpcProxy &IpcStateController::getIpcProxy() {
    return mIpcProxy;
}
