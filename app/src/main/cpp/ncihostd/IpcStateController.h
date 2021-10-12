//
// Created by kinit on 2021-10-12.
//

#ifndef NCIHOSTD_IPCSTATECONTROLLER_H
#define NCIHOSTD_IPCSTATECONTROLLER_H

#include "../rpcprotocol/IpcProxy.h"

class IpcStateController {
public:
    void attachIpcSeqPacketSocket(int fd);

    [[nodiscard]] static IpcStateController &getInstance();

private:
    IpcProxy mIpcProxy;

    void init();

    void dispatchLpcRequest();

    static bool cbLpcHandler(const IpcProxy::LpcEnv &env, LpcResult &result, uint32_t funcId, const ArgList &args);

    static void cbEventHandler(const IpcProxy::LpcEnv &env, uint32_t funcId, const ArgList &args);
};


#endif //NCIHOSTD_IPCSTATECONTROLLER_H
