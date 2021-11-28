//
// Created by kinit on 2021-10-12.
//

#include "rpcprotocol/log/Log.h"

#include "IpcStateController.h"

static const char *const LOG_TAG = "ncihostd";

using namespace ipcprotocol;

void IpcStateController::attachIpcSeqPacketSocket(int fd) {
    if (int err = mIpcTransactor.attach(fd); err != 0) {
        LOGE("IpcProxy attach fd %d error %s", fd, strerror(err));
        close(fd);
    }
    mIpcTransactor.start();
    mIpcTransactor.join();
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
    mIpcTransactor.registerIpcObject(mDaemon);
    mClientProxy.attachToIpcTransactor(mIpcTransactor);
}

NciHostDaemonImpl &IpcStateController::getNciHostDaemon() {
    return mDaemon;
}

IpcTransactor &IpcStateController::getIpcProxy() {
    return mIpcTransactor;
}

NciClientProxy &IpcStateController::getNciClientProxy() {
    return mClientProxy;
}
