//
// Created by kinit on 2021-11-22.
//
#include "IpcTransactor.h"

#include "BaseIpcProxy.h"

using namespace ipcprotocol;

bool BaseIpcProxy::isConnected() const {
    return mIpcTransactor != nullptr && (mIpcTransactor->isConnected() && mIpcTransactor->isRunning());
}

void BaseIpcProxy::attachToIpcTransactor(IpcTransactor &ipcTransactor) {
    mIpcTransactor = &ipcTransactor;
}

void BaseIpcProxy::detachFromIpcTransactor() {
    mIpcTransactor = nullptr;
}

IpcTransactor *BaseIpcProxy::getIpcTransactor() const {
    return mIpcTransactor;
}

LpcResult BaseIpcProxy::dispatchProxyInvocation(uint32_t procId, const SharedBuffer &args) {
    IpcTransactor *transactor = mIpcTransactor;
    if (transactor == nullptr) {
        LpcResult result;
        result.setError(LpcErrorCode::ERR_BROKEN_CONN);
        return result;
    }
    return transactor->executeLpcTransaction(getProxyId(), procId, 0, args);
}

LpcErrorCode BaseIpcProxy::dispatchProxyEvent(bool sync, uint32_t eventId, const SharedBuffer &args) {
    IpcTransactor *transactor = mIpcTransactor;
    if (transactor == nullptr) {
        return LpcErrorCode::ERR_BROKEN_CONN;
    }
    int err = transactor->sendEvent(sync, getProxyId(), eventId, args);
    if (err == 0) {
        return LpcErrorCode::ERR_SUCCESS;
    } else if (err == EPIPE) {
        return LpcErrorCode::ERR_BROKEN_CONN;
    } else {
        return LpcErrorCode::ERR_LOCAL_INTERNAL_ERROR;
    }
}
