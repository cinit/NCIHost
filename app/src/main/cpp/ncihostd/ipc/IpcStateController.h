//
// Created by kinit on 2021-10-12.
//

#ifndef NCIHOSTD_IPCSTATECONTROLLER_H
#define NCIHOSTD_IPCSTATECONTROLLER_H

#include "rpcprotocol/protocol/IpcTransactor.h"
#include "rpcprotocol/protocol/LpcArgListExtractor.h"
#include "../service/front/NciHostDaemonImpl.h"

namespace ipcprotocol {

class IpcStateController {
public:
    void attachIpcSeqPacketSocket(int fd);

    [[nodiscard]] static IpcStateController &getInstance();

    [[nodiscard]] NciHostDaemonImpl &getNciHostDaemon();

    [[nodiscard]] IpcTransactor &getIpcProxy();

private:
    IpcTransactor mIpcProxy;
    NciHostDaemonImpl mDaemon;

    void init();

    bool dispatchLpcRequest(const IpcTransactor::LpcEnv &env, LpcResult &result, uint32_t funcId, const ArgList &args);

    static bool cbLpcHandler(const IpcTransactor::LpcEnv &env, LpcResult &result, uint32_t funcId, const ArgList &args);

    static void cbEventHandler(const IpcTransactor::LpcEnv &env, uint32_t funcId, const ArgList &args);


};

}

#endif //NCIHOSTD_IPCSTATECONTROLLER_H
