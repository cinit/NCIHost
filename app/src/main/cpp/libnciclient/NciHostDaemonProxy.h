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

    TypedLpcResult<void> exitProcess() override;

    TypedLpcResult<int> testFunction(int value) override;
};

}

#endif //NCICLIENT_NCIHOSTDAEMONPROXY_H
