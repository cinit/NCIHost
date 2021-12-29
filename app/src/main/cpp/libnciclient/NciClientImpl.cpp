//
// Created by kinit on 2021-11-22.
//

#include "rpcprotocol/protocol/LpcArgListExtractor.h"
#include "rpcprotocol/log/Log.h"

#include "NciClientImpl.h"

using namespace ipcprotocol;
using namespace halpatch;

constexpr const char *LOG_TAG = "NciClientImpl";

uint32_t NciClientImpl::getProxyId() const {
    return PROXY_ID;
}

bool NciClientImpl::dispatchLpcInvocation(const IpcTransactor::LpcEnv &,
                                          LpcResult &, uint32_t, const ArgList &) {
    // we have no functions to dispatch
    return false;
}

bool NciClientImpl::dispatchEvent(const IpcTransactor::LpcEnv &env, uint32_t eventId, const ArgList &args) {
    using T = NciClientImpl;
    using R = LpcArgListExtractor<T>;
    using Ids = INciClient::EventIds;
    switch (eventId) {
        case Ids::IO_EVENT : {
            R::invoke(this, args, R::is(
                    +[](T *p, const IoSyscallEvent &event, const std::vector<uint8_t> &payload) {
                        p->onIoEvent(event, payload);
                    }));
            return true;
        }
        case Ids::REMOTE_DEATH: {
            R::invoke(this, args, R::is(+[](T *p, int pid) { p->onRemoteDeath(pid); }));
            return true;
        }
        default:
            return false;
    }
}

void NciClientImpl::onIoEvent(const IoSyscallEvent &event, const std::vector<uint8_t> &payload) {
    NciClientImpl_forwardRemoteIoEvent(event, payload);
}

void NciClientImpl::onRemoteDeath(int pid) {
    NciClientImpl_forwardRemoteDeathEvent(pid);
}
