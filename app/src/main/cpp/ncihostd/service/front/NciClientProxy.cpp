//
// Created by kinit on 2021-11-25.
//

#include "NciClientProxy.h"

using namespace ipcprotocol;

using Ids = INciClient::EventIds;

uint32_t NciClientProxy::getProxyId() const {
    return PROXY_ID;
}

void NciClientProxy::onIoEvent(const halpatch::IoSyscallEvent &event, const std::vector<uint8_t> &payload) {
    (void) onProxyEvent(false, Ids::IO_EVENT, event, payload);
}

void NciClientProxy::onRemoteDeath(int pid) {
    (void) onProxyEvent(true, Ids::REMOTE_DEATH, pid);
}
