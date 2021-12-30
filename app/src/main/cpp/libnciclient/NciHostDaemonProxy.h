//
// Created by kinit on 2021-10-15.
//

#ifndef NCICLIENT_NCIHOSTDAEMONPROXY_H
#define NCICLIENT_NCIHOSTDAEMONPROXY_H

#include "rpcprotocol/protocol/BaseIpcProxy.h"
#include "rpcprotocol/protocol/IpcTransactor.h"
#include "rpcprotocol/INciHostDaemon.h"

namespace ipcprotocol {

class NciHostDaemonProxy : public INciHostDaemon, public BaseIpcProxy {
public:
    NciHostDaemonProxy() = default;

    ~NciHostDaemonProxy() override = default;

    [[nodiscard]] uint32_t getProxyId() const override;

    TypedLpcResult<std::string> getVersionName() override;

    TypedLpcResult<int> getVersionCode() override;

    TypedLpcResult<std::string> getBuildUuid() override;

    TypedLpcResult<int> getSelfPid() override;

    TypedLpcResult<void> exitProcess() override;

    TypedLpcResult<bool> isDeviceSupported() override;

    TypedLpcResult<bool> isHwServiceConnected() override;

    TypedLpcResult<bool> initHwServiceConnection(const std::vector<std::string> &soPath) override;

    TypedLpcResult<HistoryIoOperationEventList> getHistoryIoOperations(uint32_t start, uint32_t length) override;

    TypedLpcResult<bool> clearHistoryIoEvents() override;

    TypedLpcResult<DaemonStatus> getDaemonStatus() override;

    TypedLpcResult<int> deviceDriverWriteRawBuffer(const std::vector<uint8_t> &buffer) override;

    TypedLpcResult<int> deviceDriverIoctl0(uint32_t request, uint64_t arg) override;

    TypedLpcResult<bool> isAndroidNfcServiceConnected() override;

    TypedLpcResult<bool> connectToAndroidNfcService() override;

    TypedLpcResult<bool> isNfcDiscoverySoundDisabled() override;

    TypedLpcResult<bool> setNfcDiscoverySoundDisabled(bool disable) override;
};

}

#endif //NCICLIENT_NCIHOSTDAEMONPROXY_H
