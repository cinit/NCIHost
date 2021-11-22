//
// Created by kinit on 2021-10-15.
//

#include "NciHostDaemonProxy.h"

using namespace ipcprotocol;

NciHostDaemonProxy::NciHostDaemonProxy() = default;

using Ids = INciHostDaemon::TransactionIds;

NciHostDaemonProxy::NciHostDaemonProxy(IpcTransactor *ipcProxy)
        : mProxy(ipcProxy) {
}

NciHostDaemonProxy::~NciHostDaemonProxy() = default;

IpcTransactor *NciHostDaemonProxy::getIpcProxy() const noexcept {
    return mProxy;
}

void NciHostDaemonProxy::setIpcProxy(IpcTransactor *ipcProxy) {
    mProxy = ipcProxy;
}

bool NciHostDaemonProxy::isConnected() const noexcept {
    return mProxy != nullptr && mProxy->isConnected();
}

TypedLpcResult<std::string> NciHostDaemonProxy::getVersionName() {
    return invokeRemoteProcedureInternal<std::string>(Ids::getVersionName);
}

TypedLpcResult<int> NciHostDaemonProxy::getVersionCode() {
    return invokeRemoteProcedureInternal<int>(Ids::getVersionCode);
}

TypedLpcResult<std::string> NciHostDaemonProxy::getBuildUuid() {
    return invokeRemoteProcedureInternal<std::string>(Ids::getBuildUuid);
}

TypedLpcResult<void> NciHostDaemonProxy::exitProcess() {
    return invokeRemoteProcedureInternal<void>(Ids::exitProcess);
}

TypedLpcResult<int> NciHostDaemonProxy::testFunction(int value) {
    return invokeRemoteProcedureInternal<int, int>(Ids::testFunction, value);
}
