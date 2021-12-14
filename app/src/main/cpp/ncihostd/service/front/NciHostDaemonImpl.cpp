//
// Created by kinit on 2021-10-13.
//

#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <string_view>

#include "ncihostd/service/hw/nxpnci/NxpHalHandler.h"
#include "ncihostd/inject/SysServicePatch.h"
#include "ncihostd/service/ServiceManager.h"
#include "rpcprotocol/utils/TextUtils.h"
#include "rpcprotocol/utils/auto_close_fd.h"
#include "rpcprotocol/utils/SELinux.h"
#include "rpcprotocol/log/Log.h"
#include "rpcprotocol/protocol/LpcArgListExtractor.h"
#include "rpcprotocol/utils/ProcessUtils.h"

#include "NciHostDaemonImpl.h"

#define LOG_TAG "NciHostDaemonImpl"

using namespace ipcprotocol;
using namespace halpatch;
using namespace hwhal;

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

TypedLpcResult<int> NciHostDaemonImpl::getSelfPid() {
    return {int(getpid())};
}

TypedLpcResult<void> NciHostDaemonImpl::exitProcess() {
    LOGI("client requested exit process, exit now...");
    exit(0);
    return {};
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
        case Ids::isDeviceSupported: {
            result = R::invoke(this, args, R::is(+[](T *p) { return p->isDeviceSupported(); }));
            return true;
        }
        case Ids::isHwServiceConnected: {
            result = R::invoke(this, args, R::is(+[](T *p) { return p->isHwServiceConnected(); }));
            return true;
        }
        case Ids::initHwServiceConnection: {
            result = R::invoke(this, args, R::is(+[](T *p, const std::string &s) {
                return p->initHwServiceConnection(s);
            }));
            return true;
        }
        case Ids::getSelfPid: {
            result = R::invoke(this, args, R::is(+[](T *p) { return p->getSelfPid(); }));
            return true;
        }
        case Ids::getHistoryIoOperations: {
            result = R::invoke(this, args, R::is(+[](T *p, uint32_t start, uint32_t length) {
                return p->getHistoryIoOperations(start, length);
            }));
            return true;
        }
        case Ids::clearHistoryIoEvents: {
            result = R::invoke(this, args, R::is(+[](T *p) { return p->clearHistoryIoEvents(); }));
            return true;
        }
        case Ids::getDaemonStatus: {
            result = R::invoke(this, args, R::is(+[](T *p) { return p->getDaemonStatus(); }));
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

TypedLpcResult<bool> NciHostDaemonImpl::isDeviceSupported() {
    HwServiceStatus status = NxpHalHandler::getHwServiceStatus();
    return status.valid;
}

TypedLpcResult<bool> NciHostDaemonImpl::isHwServiceConnected() {
    HwServiceStatus status = NxpHalHandler::getHwServiceStatus();
    return !status.currentAdapter.expired();
}

TypedLpcResult<ipcprotocol::INciHostDaemon::HistoryIoOperationEventList>
NciHostDaemonImpl::getHistoryIoOperations(uint32_t start, uint32_t length) {
    HistoryIoOperationEventList list;
    auto sp = NxpHalHandler::getWeakInstance();
    NxpHalHandler *handler;
    if (auto p = sp.get(); p != nullptr && (handler = dynamic_cast<NxpHalHandler *> (p))) {
        list = handler->getHistoryIoOperationEvents(start, length);
    }
    return {list};
}

TypedLpcResult<bool> NciHostDaemonImpl::initHwServiceConnection(const std::string &soPath) {
    // find service pid
    HwServiceStatus status = NxpHalHandler::getHwServiceStatus();
    if (!status.valid || status.serviceProcessId <= 0) {
        std::string msg = "device or HAL service is not supported";
        LOGE("%s", msg.c_str());
        return LpcResult::throwException({1, ENOTSUP, msg});
    }
    auto fd = auto_close_fd(open(soPath.c_str(), O_RDONLY));
    if (!fd) {
        int err = errno;
        std::string errMsg = "failed to open " + soPath + ": " + strerror(err);
        return LpcResult::throwException({1, uint32_t(err), errMsg});
    }
    if (auto sp = status.currentAdapter.lock()) {
        const auto *h = dynamic_cast<NxpHalHandler *>(sp.get());
        if (h && h->isConnected()) {
            // already connected
            return true;
        }
    }
    std::string shortName = [&] {
        auto s = utils::splitString(soPath, "/");
        return s[s.size() - 1];
    }();
    auto &serviceManager = ServiceManager::getInstance();
    SysServicePatch servicePatch;
    if (int err = servicePatch.init(shortName, fd.get(), NxpHalHandler::INIT_SYMBOL); err != 0) {
        std::string errMsg = "failed to init service patch: " + std::to_string(err);
        return LpcResult::throwException({1, uint32_t(err), errMsg});
    }
    int targetPid = status.serviceProcessId;
    LOGI("service patch initialized, service pid: %d", targetPid);
    if (int err = servicePatch.setSystemServicePid(targetPid); err != 0) {
        std::string errMsg = "failed to set service pid: " + std::to_string(err);
        return LpcResult::throwException({1, uint32_t(err), errMsg});
    }
    if (!servicePatch.isServicePatched()) {
        LOGI("service patch is not applied, apply it now...");
        if (int err = servicePatch.patchService(); err != 0) {
            std::string errMsg = "failed to patch service: " + std::to_string(err);
            return LpcResult::throwException({1, uint32_t(err), errMsg});
        }
    }
    int localSock = -1;
    int remoteSock = -1;
    if (int err = servicePatch.connectToService(localSock, remoteSock); err != 0) {
        std::string errMsg = "failed to connect to service: " + std::to_string(err);
        return LpcResult::throwException({1, uint32_t(err), errMsg});
    }
    NxpHalHandler::StartupInfo info = {localSock, targetPid};
    auto[result, wp] = serviceManager.startService<NxpHalHandler>(&info);
    if (result < 0) {
        std::string errMsg = "failed to start service: " + std::to_string(result);
        return LpcResult::throwException({1, uint32_t(result), errMsg});
    }
    std::shared_ptr<IBaseService> spSvc;
    NxpHalHandler *service;
    if (!(spSvc = wp.lock()) || !(service = dynamic_cast<NxpHalHandler *>(spSvc.get()))) {
        std::string errMsg = "failed to start service";
        return LpcResult::throwException({1, EFAULT, errMsg});
    }
    int currentStatus = service->getRemotePltHookStatus();
    LOGI("service started, remote status: %d", currentStatus);
    if (currentStatus == 0) {
        OriginHookProcedure hookProc = {};
        if (int err = servicePatch.getPltHookEntries(hookProc,
                                                     NxpHalHandler::TARGET_SO_NAME); err != 0) {
            std::string errMsg = "failed to get hook entries: " + std::to_string(err);
            return LpcResult::throwException({1, uint32_t(err), errMsg});
        }
        int initResult = service->initRemotePltHook(hookProc);
        if (initResult < 0) {
            std::string errMsg = "failed to init remote hook: " + std::to_string(initResult);
            return LpcResult::throwException({1, uint32_t(initResult), errMsg});
        } else {
            LOGI("remote hook initialized");
        }
    }
    return true;
}

TypedLpcResult<bool> NciHostDaemonImpl::clearHistoryIoEvents() {
    auto sp = NxpHalHandler::getWeakInstance();
    NxpHalHandler *handler;
    if (auto p = sp.get(); p != nullptr && (handler = dynamic_cast<NxpHalHandler *>(p))) {
        handler->clearHistoryIoOperationEvents();
        return {true};
    }
    return {false};
}

TypedLpcResult<INciHostDaemon::DaemonStatus> NciHostDaemonImpl::getDaemonStatus() {
    DaemonStatus status;
    status.processId = getpid();
    status.versionName = NCI_HOST_VERSION;
    std::string selfProcSecurityContext;
    if (int err = SELinux::getProcessSecurityContext(getpid(), &status.daemonProcessSecurityContext); err != 0) {
        status.daemonProcessSecurityContext = "";
        LOGE("failed to get self process security context: %d", err);
    }
    status.abiArch = utils::getCurrentProcessArchitecture();
    auto sp = NxpHalHandler::getWeakInstance();
    const NxpHalHandler *handler;
    if (auto p = sp.get(); p != nullptr && (handler = dynamic_cast<NxpHalHandler *>(p)) && handler->isConnected()) {
        // HAL service is attached
        status.isHalServiceAttached = true;
        status.halServicePid = handler->getRemotePid();
    } else {
        status.isHalServiceAttached = false;
        auto unHwStatus = NxpHalHandler::getHwServiceStatus();
        status.halServicePid = unHwStatus.serviceProcessId;
    }
    if (status.halServicePid > 0) {
        if (utils::ProcessInfo procInfo = {}; utils::getProcessInfo(status.halServicePid, procInfo)) {
            status.halServiceUid = procInfo.uid;
            status.halServiceExePath = procInfo.exe;
        } else {
            LOGE("failed to get process info for pid: %d", status.halServicePid);
            status.halServiceUid = -1;
            status.halServiceExePath = "";
        }
        elfsym::ProcessView procView;
        if (int err = procView.readProcess(status.halServicePid); err == 0) {
            status.halServiceArch = procView.getArchitecture();
        } else {
            LOGE("failed to read process info for pid: %d, error: %d", status.halServicePid, err);
            status.halServiceArch = -1;
        }
        if (int err = SELinux::getProcessSecurityContext(status.halServicePid,
                                                         &status.halServiceProcessSecurityContext); err != 0) {
            LOGE("failed to get process security context for pid: %d, error: %d", status.halServicePid, err);
            status.halServiceProcessSecurityContext = "";
        }
        if (!status.halServiceExePath.empty()) {
            std::string label;
            if (int err = SELinux::getFileSEContext(status.halServiceExePath.c_str(), &label); err == 0) {
                status.halServiceExecutableSecurityLabel = label;
            } else {
                LOGE("failed to get file security context for path: %s, error: %d",
                     status.halServiceExePath.c_str(), err);
                status.halServiceExecutableSecurityLabel = "";
            }
        } else {
            status.halServiceExecutableSecurityLabel = "";
        }
    } else {
        status.halServiceUid = -1;
        status.halServiceExePath = "";
        status.halServiceArch = -1;
    }
    return {status};
}
