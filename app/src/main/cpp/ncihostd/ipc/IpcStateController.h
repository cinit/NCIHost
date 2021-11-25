//
// Created by kinit on 2021-10-12.
//

#ifndef NCIHOSTD_IPCSTATECONTROLLER_H
#define NCIHOSTD_IPCSTATECONTROLLER_H

#include "rpcprotocol/protocol/IpcTransactor.h"
#include "rpcprotocol/protocol/LpcArgListExtractor.h"
#include "../service/front/NciHostDaemonImpl.h"
#include "../service/front/NciClientProxy.h"

namespace ipcprotocol {

class IpcStateController {
public:
    void attachIpcSeqPacketSocket(int fd);

    [[nodiscard]] static IpcStateController &getInstance();

    [[nodiscard]] NciHostDaemonImpl &getNciHostDaemon();

    [[nodiscard]] IpcTransactor &getIpcProxy();

private:
    IpcTransactor mIpcTransactor;
    NciHostDaemonImpl mDaemon;
    NciClientProxy mClientProxy;

    void init();
};

}

#endif //NCIHOSTD_IPCSTATECONTROLLER_H
