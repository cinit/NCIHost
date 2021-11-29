//
// Created by kinit on 2021-11-22.
//

#ifndef NCI_HOST_NATIVES_NCICLIENTIMPL_H
#define NCI_HOST_NATIVES_NCICLIENTIMPL_H

#include "rpcprotocol/INciClient.h"
#include "rpcprotocol/protocol/BaseIpcObject.h"

namespace ipcprotocol {

class NciClientImpl : public INciClient, public BaseIpcObject {
public:
    NciClientImpl() = default;

    ~NciClientImpl() override = default;

    NciClientImpl(const NciClientImpl &) = delete;

    NciClientImpl &operator=(const NciClientImpl &) = delete;

    [[nodiscard]] uint32_t getProxyId() const override;

    bool dispatchLpcInvocation(const IpcTransactor::LpcEnv &env, LpcResult &result,
                               uint32_t funcId, const ArgList &args) override;

    bool dispatchEvent(const IpcTransactor::LpcEnv &env, uint32_t eventId, const ArgList &args) override;

    void onIoEvent(const halpatch::IoOperationEvent &event, const std::vector<uint8_t> &payload) override;

    void onRemoteDeath(int pid) override;
};

}

void NciClientImpl_forwardRemoteIoEvent(const halpatch::IoOperationEvent &event, const std::vector<uint8_t> &payload);

void NciClientImpl_forwardRemoteDeathEvent(int pid);

#endif //NCI_HOST_NATIVES_NCICLIENTIMPL_H
