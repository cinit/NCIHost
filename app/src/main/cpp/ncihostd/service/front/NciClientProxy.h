//
// Created by kinit on 2021-11-25.
//

#ifndef NCI_HOST_NATIVES_NCICLIENTPROXY_H
#define NCI_HOST_NATIVES_NCICLIENTPROXY_H

#include "libbasehalpatch/ipc/daemon_ipc_struct.h"
#include "rpcprotocol/protocol/IpcTransactor.h"
#include "rpcprotocol/protocol/BaseIpcProxy.h"
#include "rpcprotocol/INciClient.h"

namespace ipcprotocol {

class NciClientProxy : public INciClient, public BaseIpcProxy {
public:
    NciClientProxy() = default;

    ~NciClientProxy() override = default;

    [[nodiscard]] uint32_t getProxyId() const override;

    NciClientProxy(const NciClientProxy &) = delete;

    NciClientProxy &operator=(const NciClientProxy &) = delete;

    void onIoEvent(const halpatch::IoSyscallEvent &event, const std::vector<uint8_t> &payload) override;

    void onRemoteDeath(int pid) override;
};

}

#endif //NCI_HOST_NATIVES_NCICLIENTPROXY_H
