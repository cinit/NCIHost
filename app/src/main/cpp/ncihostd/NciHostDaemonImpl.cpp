//
// Created by kinit on 2021-10-13.
//

#include "NciHostDaemonImpl.h"
#include "../rpcprotocol/Log.h"
#include "../rpcprotocol/LpcArgListExtractor.h"

#define LOG_TAG "ncihostd"

using namespace ipcprotocol;

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

bool NciHostDaemonImpl::dispatchLpcInvocation([[maybe_unused]] const IpcProxy::LpcEnv &env,
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
