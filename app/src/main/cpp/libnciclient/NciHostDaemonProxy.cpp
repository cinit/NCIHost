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

TypedLpcResult<int> NciHostDaemonProxy::getSelfPid() {
    return invokeRemoteProcedure<int>(Ids::getSelfPid);
}

TypedLpcResult<void> NciHostDaemonProxy::exitProcess() {
    return invokeRemoteProcedure<void>(Ids::exitProcess);
}

TypedLpcResult<bool> NciHostDaemonProxy::isDeviceSupported() {
    return invokeRemoteProcedure<bool>(Ids::isDeviceSupported);
}

TypedLpcResult<bool> NciHostDaemonProxy::isHwServiceConnected() {
    return invokeRemoteProcedure<bool>(Ids::isHwServiceConnected);
}

TypedLpcResult<bool> NciHostDaemonProxy::initHwServiceConnection(const std::vector<std::string> &soPath) {
    return invokeRemoteProcedure<bool, const std::vector<std::string> &>(Ids::initHwServiceConnection, soPath);
}

TypedLpcResult<INciHostDaemon::HistoryIoOperationEventList>
NciHostDaemonProxy::getHistoryIoOperations(uint32_t start, uint32_t length) {
    return invokeRemoteProcedure<INciHostDaemon::HistoryIoOperationEventList, const uint32_t &, const uint32_t &>(
            Ids::getHistoryIoOperations, start, length);
}

TypedLpcResult<bool> NciHostDaemonProxy::clearHistoryIoEvents() {
    return invokeRemoteProcedure<bool>(Ids::clearHistoryIoEvents);
}

TypedLpcResult<INciHostDaemon::DaemonStatus> NciHostDaemonProxy::getDaemonStatus() {
    return invokeRemoteProcedure<INciHostDaemon::DaemonStatus>(Ids::getDaemonStatus);
}

TypedLpcResult<int> NciHostDaemonProxy::deviceDriverWriteRawBuffer(const std::vector<uint8_t> &buffer) {
    return invokeRemoteProcedure<int, const std::vector<uint8_t> &>(Ids::deviceDriverWriteRawBuffer, buffer);
}

TypedLpcResult<int> NciHostDaemonProxy::deviceDriverIoctl0(uint64_t request, uint64_t arg) {
    return invokeRemoteProcedure<int, const uint64_t &, const uint64_t &>(Ids::deviceDriverIoctl0, request, arg);
}

TypedLpcResult<bool> NciHostDaemonProxy::isAndroidNfcServiceConnected() {
    return invokeRemoteProcedure<bool>(Ids::isAndroidNfcServiceConnected);
}

TypedLpcResult<bool> NciHostDaemonProxy::connectToAndroidNfcService() {
    return invokeRemoteProcedure<bool>(Ids::connectToAndroidNfcService);
}

TypedLpcResult<bool> NciHostDaemonProxy::isNfcDiscoverySoundDisabled() {
    return invokeRemoteProcedure<bool>(Ids::isNfcDiscoverySoundDisabled);
}

TypedLpcResult<bool> NciHostDaemonProxy::setNfcDiscoverySoundDisabled(bool disable) {
    return invokeRemoteProcedure<bool, const bool &>(Ids::setNfcDiscoverySoundDisabled, disable);
}
