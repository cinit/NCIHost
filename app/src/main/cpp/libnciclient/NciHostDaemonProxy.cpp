//
// Created by kinit on 2021-10-15.
//

#include "NciHostDaemonProxy.h"

using namespace ipcprotocol;

using Ids = INciHostDaemon::TransactionIds;

uint32_t NciHostDaemonProxy::getProxyId() const {
    return PROXY_ID;
}

TypedLpcResult<std::string> NciHostDaemonProxy::getVersionName() {
    return invokeRemoteProcedure<std::string>(Ids::getVersionName);
}

TypedLpcResult<int> NciHostDaemonProxy::getVersionCode() {
    return invokeRemoteProcedure<int>(Ids::getVersionCode);
}

TypedLpcResult<std::string> NciHostDaemonProxy::getBuildUuid() {
    return invokeRemoteProcedure<std::string>(Ids::getBuildUuid);
}

TypedLpcResult<void> NciHostDaemonProxy::exitProcess() {
    return invokeRemoteProcedure<void>(Ids::exitProcess);
}

TypedLpcResult<int> NciHostDaemonProxy::testFunction(int value) {
    return invokeRemoteProcedure<int, int>(Ids::testFunction, value);
}
