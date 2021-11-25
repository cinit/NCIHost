//
// Created by kinit on 2021-10-13.
//
#include "../../ipc/IpcStateController.h"
#include "NciHostDaemonImpl.h"
#include "rpcprotocol/log/Log.h"
#include "rpcprotocol/protocol/LpcArgListExtractor.h"

#define LOG_TAG "ncihostd"

using namespace ipcprotocol;
using namespace halpatch;

uint32_t NciHostDaemonImpl::getProxyId() const {
    return PROXY_ID;
}

TypedLpcResult<std::string> NciHostDaemonImpl::getVersionName() {
    return {NCI_HOST_VERSION};
}

TypedLpcResult<int> NciHostDaemonImpl::getVersionCode() {
    return {1};
}

TypedLpcResult<std::string> NciHostDaemonImpl::getBuildUuid() {
    return {"NOT_AVAILABLE"};
}

TypedLpcResult<void> NciHostDaemonImpl::exitProcess() {
    LOGI("client requested exit process, exit now...");
    exit(0);
    return {};
}

TypedLpcResult<int> NciHostDaemonImpl::testFunction(int value) {
    return {value / 2};
}

bool NciHostDaemonImpl::dispatchLpcInvocation([[maybe_unused]] const IpcTransactor::LpcEnv &env,
                                              LpcResult &result, uint32_t funcId, const ArgList &args) {
    using T = NciHostDaemonImpl;
    using R = LpcArgListExtractor<T>;
    using Ids = NciHostDaemonImpl::TransactionIds;
    switch (funcId) {
        case Ids::getVersionName: {
            result = R::invoke(this, args, R::is(+[](T *p) { return p->getVersionName(); }));
            return true;
        }
        case Ids::getVersionCode: {
            result = R::invoke(this, args, R::is(+[](T *p) { return p->getVersionCode(); }));
            return true;
        }
        case Ids::getBuildUuid: {
            result = R::invoke(this, args, R::is(+[](T *p) { return p->getBuildUuid(); }));
            return true;
        }
        case Ids::exitProcess: {
            result = R::invoke(this, args, R::is(+[](T *p) { return p->exitProcess(); }));
            return true;
        }
        case Ids::testFunction: {
            result = R::invoke(this, args, R::is(+[](T *p, int v1) { return p->testFunction(v1); }));
            return true;
        }
        default:
            return false;
    }
}

bool NciHostDaemonImpl::dispatchEvent(const IpcTransactor::LpcEnv &env, uint32_t eventId, const ArgList &args) {
    // we have no events to handle
    return false;
}

int NciHostDaemonImpl::sendIoOperationEvent(const IoOperationEvent &event, const void *payload) {
    IpcTransactor &proxy = IpcStateController::getInstance().getIpcProxy();
    if (proxy.isConnected() && proxy.isRunning()) {
        ArgList::Builder args;
        std::vector<uint8_t> eventBytes;
        std::vector<uint8_t> payloadBytes;
        memcpy(&eventBytes[0], &event, sizeof(event));
        if (payload != nullptr && event.info.bufferLength > 0) {
            memcpy(&payloadBytes[0], payload, event.info.bufferLength);
        }
        args.push(eventBytes);
        if (!payloadBytes.empty()) {
            args.push(payloadBytes);
        }
        return proxy.sendEvent(false, PROXY_ID, EventIds::IO_EVENT, args.build());
    } else {
        return EPIPE;
    }
}

int NciHostDaemonImpl::sendRemoteDeathEvent(int pid) {
    IpcTransactor &proxy = IpcStateController::getInstance().getIpcProxy();
    if (proxy.isConnected() && proxy.isRunning()) {
        ArgList::Builder args;
        args.push(pid);
        return proxy.sendEvent(false, PROXY_ID, EventIds::REMOTE_DEATH, args.build());
    } else {
        return EPIPE;
    }
}
