//
// Created by kinit on 2021-10-13.
//

#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <cerrno>
#include <string_view>

#include "ncihostd/service/hw/nxpnci/NxpHalHandler.h"
#include "ncihostd/service/hw/qtiesepm/QtiEsePmHandler.h"
#include "ncihostd/inject/SysServicePatch.h"
#include "ncihostd/service/ServiceManager.h"
#include "ncihostd/service/xposed/AndroidNfcService.h"
#include "ncihostd/ipc/IpcStateController.h"

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

static constexpr uint32_t MAX_HISTORY_EVENT_SIZE = 3000;

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
            result = R::invoke(this, args, R::is(+[](T *p, const std::vector<std::string> &s) {
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
        case Ids::deviceDriverWriteRawBuffer: {
            result = R::invoke(this, args, R::is(+[](T *p, const std::vector<uint8_t> &buffer) {
                return p->deviceDriverWriteRawBuffer(buffer);
            }));
            return true;
        }
        case Ids::deviceDriverIoctl0: {
            result = R::invoke(this, args, R::is(+[](T *p, uint64_t req, uint64_t arg) {
                return p->deviceDriverIoctl0(req, arg);
            }));
            return true;
        }
        case Ids::isAndroidNfcServiceConnected: {
            result = R::invoke(this, args, R::is(+[](T *p) { return p->isAndroidNfcServiceConnected(); }));
            return true;
        }
        case Ids::connectToAndroidNfcService: {
            result = R::invoke(this, args, R::is(+[](T *p) { return p->connectToAndroidNfcService(); }));
            return true;
        }
        case Ids::isNfcDiscoverySoundDisabled: {
            result = R::invoke(this, args, R::is(+[](T *p) { return p->isNfcDiscoverySoundDisabled(); }));
            return true;
        }
        case Ids::setNfcDiscoverySoundDisabled: {
            result = R::invoke(this, args, R::is(+[](T *p, bool disabled) {
                return p->setNfcDiscoverySoundDisabled(disabled);
            }));
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

void NciHostDaemonImpl::handleIoSyscallEvent(const IoSyscallEvent &event, const std::vector<uint8_t> &payload) {
    {
        // add the event to the queue tail
        std::scoped_lock lock(mEventMutex);
        mHistoryIoEvents.emplace_back(event, payload);
        // drop the oldest event if the queue is full
        if (mHistoryIoEvents.size() > MAX_HISTORY_EVENT_SIZE) {
            mHistoryIoEvents.pop_front();
        }
    }
    // notify the client
    IpcStateController::getInstance().getNciClientProxy().onIoEvent(event, payload);
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
NciHostDaemonImpl::getHistoryIoOperations(uint32_t start, uint32_t count) {
    INciHostDaemon::HistoryIoOperationEventList result;
    std::scoped_lock lock(mEventMutex);
    if (mHistoryIoEvents.empty()) {
        result.totalStartIndex = 0;
        result.totalCount = 0;
        return {result};
    }
    // history event list is not empty
    result.totalStartIndex = std::get<0>(mHistoryIoEvents.front()).uniqueSequence;
    result.totalCount = uint32_t(mHistoryIoEvents.size());
    // iterate the history events
    auto it = mHistoryIoEvents.cbegin();
    while (it != mHistoryIoEvents.cend()) {
        auto &[event, payload] = *it;
        if (event.uniqueSequence >= start) {
            // found the start index
            if (result.events.size() >= count) {
                break;
            }
            // add the event to the result list
            result.events.emplace_back(event);
            result.payloads.emplace_back(payload);
        }
        it++;
    }
    return {result};
}

template<typename TargetHalHandler>
bool getHwServiceSupportStatus(HwServiceStatus &halStatus, LpcResult &lpcResult, bool &isConnected) {
    halStatus = TargetHalHandler::getHwServiceStatus();
    if (!halStatus.valid || halStatus.serviceProcessId <= 0) {
        std::string msg = std::string(typeid(TargetHalHandler).name()) + ": device or HAL service is not supported";
        LOGE("%s", msg.c_str());
        lpcResult = LpcResult::throwException({1, ENOTSUP, msg});
        isConnected = false;
        return false;
    }
    if (auto sp = halStatus.currentAdapter.lock()) {
        const auto *h = dynamic_cast<TargetHalHandler *>(sp.get());
        if (h && h->isConnected()) {
            // already connected
            isConnected = true;
            return true;
        }
    }
    isConnected = false;
    return true;
}

template<typename TargetHalHandler>
bool injectIntoVendorHalService(LpcResult &lpcResult, const HwServiceStatus &halStatus, const std::string &soPatchPath,
                                const std::string &targetSoName, uint32_t hookFunctions) {
    auto fd = auto_close_fd(open(soPatchPath.c_str(), O_RDONLY));
    if (!fd) {
        int err = errno;
        std::string errMsg = "failed to open " + soPatchPath + ": " + strerror(err);
        lpcResult = LpcResult::throwException({1, uint32_t(err), errMsg});
        return false;
    }
    std::string shortName = [&] {
        auto s = utils::splitString(soPatchPath, "/");
        return s[s.size() - 1];
    }();
    auto &serviceManager = ServiceManager::getInstance();
    SysServicePatch servicePatch;
    if (int err = servicePatch.init(shortName, fd.get(), TargetHalHandler::INIT_SYMBOL); err != 0) {
        std::string errMsg = "failed to init service patch: " + std::to_string(err);
        lpcResult = LpcResult::throwException({1, uint32_t(err), errMsg});
        return false;
    }
    int targetPid = halStatus.serviceProcessId;
    LOGI("service patch initialized, service pid: %d", targetPid);
    if (int err = servicePatch.setSystemServicePid(targetPid); err != 0) {
        std::string errMsg = "failed to set service pid: " + std::to_string(err);
        lpcResult = LpcResult::throwException({1, uint32_t(err), errMsg});
        return false;
    }
    if (!servicePatch.isServicePatched()) {
        LOGI("service patch is not applied, apply it now...");
        if (int err = servicePatch.patchService(); err != 0) {
            std::string errMsg = "failed to patch service: " + std::to_string(err);
            lpcResult = LpcResult::throwException({1, uint32_t(err), errMsg});
            return false;
        }
    }
    int localSock = -1;
    int remoteSock = -1;
    if (int err = servicePatch.connectToService(localSock, remoteSock); err != 0) {
        std::string errMsg = "failed to connect to service: " + std::to_string(err);
        lpcResult = LpcResult::throwException({1, uint32_t(err), errMsg});
        return false;
    }
    typename TargetHalHandler::StartupInfo info = {localSock, targetPid};
    auto[result, wp] = serviceManager.startService<TargetHalHandler>(&info);
    if (result < 0) {
        std::string errMsg = "failed to start service: " + std::to_string(result);
        lpcResult = LpcResult::throwException({1, uint32_t(result), errMsg});
        return false;
    }
    std::shared_ptr<IBaseService> spSvc;
    TargetHalHandler *service;
    if (!(spSvc = wp.lock()) || !(service = dynamic_cast<TargetHalHandler *>(spSvc.get()))) {
        std::string errMsg = "failed to start service";
        lpcResult = LpcResult::throwException({1, EFAULT, errMsg});
        return false;
    }
    int currentStatus = service->getRemotePltHookStatus();
    LOGI("service started, remote status: %d", currentStatus);
    if (currentStatus == 0) {
        OriginHookProcedure hookProc = {};
        if (int err = servicePatch.getPltHookEntries(hookProc, targetSoName, hookFunctions); err != 0) {
            std::string errMsg = "failed to get hook entries: " + std::to_string(err);
            lpcResult = LpcResult::throwException({1, uint32_t(err), errMsg});
            return false;
        }
        int initResult = service->initRemotePltHook(hookProc);
        if (initResult < 0) {
            std::string errMsg = "failed to init remote hook: " + std::to_string(initResult);
            lpcResult = LpcResult::throwException({1, uint32_t(initResult), errMsg});
            return false;
        } else {
            LOGI("remote hook initialized");
        }
    }
    return true;
}

TypedLpcResult<bool> NciHostDaemonImpl::initHwServiceConnection(const std::vector<std::string> &soPath) {
    // check argument count
    if (soPath.size() < 2) {
        std::string msg = "invalid arguments: required at least 2 strings: soPath_nxphalpatch, soPath_qtiesepatch";
        LOGE("%s", msg.c_str());
        return LpcResult::throwException({1, EINVAL, msg});
    }
    std::string soPath_nxphalpatch = soPath[0];
    std::string soPath_qtiesepatch = soPath[1];
    // check absolute path
    if (soPath_nxphalpatch.find('/') != 0 || soPath_qtiesepatch.find('/') != 0) {
        std::string msg = "invalid arguments: soPath_nxphalpatch or soPath_qtiesepatch is not absolute path";
        LOGE("%s", msg.c_str());
        return LpcResult::throwException({1, EINVAL, msg});
    }
    {
        using T = SysServicePatch::PltHookTarget;
        constexpr uint32_t nfcHalHook = T::OPEN | T::CLOSE | T::READ | T::WRITE | T::IOCTL;
        LpcResult tmpResult;
        HwServiceStatus halStatus = {};
        bool connected = false;
        if (!getHwServiceSupportStatus<NxpHalHandler>(halStatus, tmpResult, connected)) {
            // hw hal not supported
            return tmpResult;
        }
        if (!connected && !injectIntoVendorHalService<NxpHalHandler>(tmpResult, halStatus, soPath_nxphalpatch,
                                                                     NxpHalHandler::TARGET_SO_NAME, nfcHalHook)) {
            return tmpResult;

        }
        if (!getHwServiceSupportStatus<QtiEsePmHandler>(halStatus, tmpResult, connected)) {
            // hw hal not supported
            return tmpResult;
        }
        if (!connected) {
            constexpr uint32_t qtiEseHalHook = T::OPEN | T::CLOSE | T::IOCTL;
            std::string shortExeName = halStatus.serviceExecPath
                    .substr(halStatus.serviceExecPath.find_last_of('/') + 1);
            // exe: vendor.qti.esepowermanager@X.Y-service
            // so: vendor.qti.esepowermanager@X.Y-impl.so
            std::string soName = shortExeName.substr(0, shortExeName.find_last_of('-')) + "-impl.so";
            if (!injectIntoVendorHalService<QtiEsePmHandler>(tmpResult, halStatus, soPath_qtiesepatch,
                                                             soName, qtiEseHalHook)) {
                return tmpResult;
            }
        }
    }
    {
        auto &androidNfcSvc = androidsvc::AndroidNfcService::getInstance();
        if (!androidNfcSvc.isConnected()) {
            // we don't care about the result, the client will check the status later
            (void) androidNfcSvc.tryConnect();
        }
    }
    return true;
}

TypedLpcResult<bool> NciHostDaemonImpl::clearHistoryIoEvents() {
    std::scoped_lock lock(mEventMutex);
    bool isEmpty = mHistoryIoEvents.empty();
    mHistoryIoEvents.clear();
    return {!isEmpty};
}

template<typename TargetHalHandler>
static void fillInHalServiceStatus(INciHostDaemon::DaemonStatus::HalServiceStatus &svcStatus) {
    auto sp = TargetHalHandler::getWeakInstance();
    const TargetHalHandler *handler;
    if (auto p = sp.get(); p != nullptr && (handler = dynamic_cast<TargetHalHandler *>(p)) && handler->isConnected()) {
        // HAL service is attached
        svcStatus.isHalServiceAttached = true;
        svcStatus.halServicePid = handler->getRemotePid();
    } else {
        svcStatus.isHalServiceAttached = false;
        auto unHwStatus = TargetHalHandler::getHwServiceStatus();
        svcStatus.halServicePid = unHwStatus.serviceProcessId;
    }
    if (svcStatus.halServicePid > 0) {
        if (utils::ProcessInfo procInfo = {}; utils::getProcessInfo(svcStatus.halServicePid, procInfo)) {
            svcStatus.halServiceUid = procInfo.uid;
            svcStatus.halServiceExePath = procInfo.exe;
        } else {
            LOGE("failed to get process info for pid: %d", svcStatus.halServicePid);
            svcStatus.halServiceUid = -1;
            svcStatus.halServiceExePath = "";
        }
        elfsym::ProcessView procView;
        if (int err = procView.readProcess(svcStatus.halServicePid); err == 0) {
            svcStatus.halServiceArch = procView.getArchitecture();
        } else {
            LOGE("failed to read process info for pid: %d, error: %d", svcStatus.halServicePid, err);
            svcStatus.halServiceArch = -1;
        }
        if (int err = SELinux::getProcessSecurityContext(svcStatus.halServicePid,
                                                         &svcStatus.halServiceProcessSecurityContext); err != 0) {
            LOGE("failed to get process security context for pid: %d, error: %d", svcStatus.halServicePid, err);
            svcStatus.halServiceProcessSecurityContext = "";
        }
        if (!svcStatus.halServiceExePath.empty()) {
            std::string label;
            if (int err = SELinux::getFileSEContext(svcStatus.halServiceExePath.c_str(), &label); err == 0) {
                svcStatus.halServiceExecutableSecurityLabel = label;
            } else {
                LOGE("failed to get file security context for path: %s, error: %d",
                     svcStatus.halServiceExePath.c_str(), err);
                svcStatus.halServiceExecutableSecurityLabel = "";
            }
        } else {
            svcStatus.halServiceExecutableSecurityLabel = "";
        }
    } else {
        svcStatus.halServiceUid = -1;
        svcStatus.halServiceExePath = "";
        svcStatus.halServiceArch = -1;
    }
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
    fillInHalServiceStatus<NxpHalHandler>(status.nfcHalServiceStatus);
    fillInHalServiceStatus<QtiEsePmHandler>(status.esePmServiceStatus);
    return {status};
}

TypedLpcResult<int> NciHostDaemonImpl::deviceDriverWriteRawBuffer(const std::vector<uint8_t> &buffer) {
    auto sp = NxpHalHandler::getWeakInstance();
    NxpHalHandler *handler;
    if (auto p = sp.get(); p != nullptr && (handler = dynamic_cast<NxpHalHandler *>(p))) {
        int ret = handler->driverRawWrite(buffer);
        return {ret};
    } else {
        return {-ENOSYS};
    }
}

TypedLpcResult<int> NciHostDaemonImpl::deviceDriverIoctl0(uint64_t request, uint64_t arg) {
    auto sp = NxpHalHandler::getWeakInstance();
    NxpHalHandler *handler;
    if (auto p = sp.get(); p != nullptr && (handler = dynamic_cast<NxpHalHandler *>(p))) {
        int ret = handler->driverRawIoctl0(request, arg);
        return {ret};
    } else {
        return {-ENOSYS};
    }
}

TypedLpcResult<bool> NciHostDaemonImpl::isAndroidNfcServiceConnected() {
    const auto &androidNfcService = androidsvc::AndroidNfcService::getInstance();
    return {androidNfcService.isConnected()};
}

TypedLpcResult<bool> NciHostDaemonImpl::connectToAndroidNfcService() {
    auto &androidNfcService = androidsvc::AndroidNfcService::getInstance();
    return {androidNfcService.tryConnect()};
}

TypedLpcResult<bool> NciHostDaemonImpl::isNfcDiscoverySoundDisabled() {
    auto &androidNfcService = androidsvc::AndroidNfcService::getInstance();
    return {androidNfcService.isNfcDiscoverySoundDisabled()};
}

TypedLpcResult<bool> NciHostDaemonImpl::setNfcDiscoverySoundDisabled(bool disable) {
    auto &androidNfcService = androidsvc::AndroidNfcService::getInstance();
    return {androidNfcService.setNfcDiscoverySoundDisabled(disable)};
}
