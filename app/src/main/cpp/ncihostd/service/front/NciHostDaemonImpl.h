//
// Created by kinit on 2021-10-13.
//

#ifndef NCIHOSTD_NCIHOSTDAEMONIMPL_H
#define NCIHOSTD_NCIHOSTDAEMONIMPL_H

#include "libbasehalpatch/ipc/daemon_ipc_struct.h"
#include "rpcprotocol/protocol/IpcTransactor.h"
#include "rpcprotocol/protocol/BaseIpcObject.h"
#include "rpcprotocol/INciHostDaemon.h"

namespace ipcprotocol {

class NciHostDaemonImpl : public INciHostDaemon, public BaseIpcObject {
public:
    NciHostDaemonImpl() = default;

    ~NciHostDaemonImpl() override = default;

    NciHostDaemonImpl(const NciHostDaemonImpl &) = delete;

    NciHostDaemonImpl &operator=(const NciHostDaemonImpl &) = delete;

    [[nodiscard]] uint32_t getProxyId() const override;

    bool dispatchLpcInvocation(const IpcTransactor::LpcEnv &env, LpcResult &result,
                               uint32_t funcId, const ArgList &args) override;

    bool dispatchEvent(const IpcTransactor::LpcEnv &env, uint32_t eventId, const ArgList &args) override;

    TypedLpcResult<std::string> getVersionName() override;

    TypedLpcResult<int> getVersionCode() override;

    TypedLpcResult<std::string> getBuildUuid() override;

    TypedLpcResult<void> exitProcess() override;

    TypedLpcResult<int> testFunction(int value) override;

    int sendIoOperationEvent(const halpatch::IoOperationEvent &event, const void *payload);

    int sendRemoteDeathEvent(int pid);

private:
};

}

#endif //NCIHOSTD_NCIHOSTDAEMONIMPL_H
