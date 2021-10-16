//
// Created by kinit on 2021-10-13.
//

#ifndef NCIHOSTD_NCIHOSTDAEMONIMPL_H
#define NCIHOSTD_NCIHOSTDAEMONIMPL_H

#include "../rpcprotocol/INciHostDaemon.h"
#include "../rpcprotocol/IpcProxy.h"

namespace ipcprotocol {

class NciHostDaemonImpl : public INciHostDaemon {
public:
    ~NciHostDaemonImpl() override = default;

    bool dispatchLpcInvocation(const IpcProxy::LpcEnv &env, LpcResult &result, uint32_t funcId, const ArgList &args);

    TypedLpcResult<std::string> getVersionName() override;

    TypedLpcResult<int> getVersionCode() override;

    TypedLpcResult<std::string> getBuildUuid() override;

    TypedLpcResult<void> exitProcess() override;

    TypedLpcResult<int> testFunction(int value) override;

private:
};

}

#endif //NCIHOSTD_NCIHOSTDAEMONIMPL_H
