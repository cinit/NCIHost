//
// Created by kinit on 2021-11-17.
//
#include <string>

#include "libqtiesepmpatch/ipc/ipc_requests.h"
#include "libbasehalpatch/ipc/daemon_ipc_struct.h"
#include "rpcprotocol/log/Log.h"
#include "rpcprotocol/utils/ProcessUtils.h"
#include "../../../ipc/IpcStateController.h"

#include "QtiEsePmHandler.h"

using namespace hwhal;
using namespace halpatch;
using namespace halpatch::qtiesepm;
using namespace ipcprotocol;

static const char *const LOG_TAG = "QtiEsePmHandler";

std::weak_ptr<IBaseService> QtiEsePmHandler::sWpInstance;

std::string_view QtiEsePmHandler::getName() const noexcept {
    return "QtiEsePmHandler";
}

std::string_view QtiEsePmHandler::getDescription() const noexcept {
    return mDescription;
}

int QtiEsePmHandler::doOnStart(void *args, const std::shared_ptr<IBaseService> &sp) {
    mDescription = "QtiEsePmHandler@fd" + std::to_string(getFd()) + "-pid-" + std::to_string(getRemotePid());
    sWpInstance = sp;
    LOGI("start: %s", mDescription.c_str());
    return 0;
}

bool QtiEsePmHandler::doOnStop() {
    return true;
}

void QtiEsePmHandler::dispatchHwHalIoEvent(const IoSyscallEvent &event, const void *payload) {
    std::vector<uint8_t> payloadVec;
    if (payload != nullptr && event.info.bufferLength > 0) {
        payloadVec.assign(static_cast<const uint8_t *>(payload),
                          static_cast<const uint8_t *>(payload) + event.info.bufferLength);
    }
    IoSyscallEvent eventCopy = event;
    NciHostDaemonImpl &daemonImpl = IpcStateController::getInstance().getNciHostDaemon();
    // fill the missing fields
    eventCopy.uniqueSequence = daemonImpl.nextIoSyscallEventSequence();
    eventCopy.sourceType = SourceType::SECURE_ELEMENT_EMBEDDED;
    daemonImpl.handleIoSyscallEvent(eventCopy, payloadVec);
}

void QtiEsePmHandler::dispatchRemoteProcessDeathEvent() {
    LOGW("Remote process death event received, pid: %d", getRemotePid());
    IpcStateController::getInstance().getNciClientProxy().onRemoteDeath(getRemotePid());
}

int QtiEsePmHandler::getRemotePltHookStatus() {
    HalRequest request = {};
    request.requestCode = static_cast<uint32_t>(RequestId::GET_HOOK_STATUS);
    auto[rc, response, payload] = sendHalRequest(request, nullptr, 1000);
    if (rc < 0) {
        LOGE("Failed to send request, rc=%d", rc);
        return rc;
    }
    return int(response.result);
}

int QtiEsePmHandler::initRemotePltHook(const OriginHookProcedure &hookProc) {
    HalRequest request = {};
    request.requestCode = static_cast<uint32_t>(RequestId::INIT_PLT_HOOK);
    request.payloadSize = sizeof(hookProc);
    auto[rc, response, payload] = sendHalRequest(request, &hookProc, 1000);
    if (rc < 0) {
        LOGE("Failed to send request, rc=%d", rc);
        return rc;
    }
    if (response.error != 0) {
        std::string errorMsg;
        if (response.payloadSize != 0 && !payload.empty()) {
            errorMsg = std::string(reinterpret_cast<const char *>(payload.data()), payload.size());
        }
        LOGE("Failed to init remote PLT hook, remote error=%d, %s", response.error, errorMsg.c_str());
        return -int(response.error);
    }
    return int(response.result);
}

HwServiceStatus QtiEsePmHandler::getHwServiceStatus() {
    HwServiceStatus status = {};
    // test hw device compatibility
    if (access(DEV_PATH, F_OK) != 0) {
        status.valid = false;
        return status;
    }
    // test hal driver service compatibility
    auto processList = utils::getRunningProcessInfo();
    if (processList.empty()) {
        LOGE("Unable to get running process info");
        status.valid = false;
        return status;
    }
    for (const auto &p: processList) {
        if (!p.name.empty() && p.uid >= 1000 && p.uid < 10000) {
            bool nameMatch = false;
            std::vector<std::string> availableVersions = {"2.0", "2.1", "1.2", "1.1", "1.0"};
            for (const auto &v: availableVersions) {
                if (p.name == (std::string(EXEC_NAME_BASE) + v + std::string(EXEC_NAME_SUFFIX))) {
                    nameMatch = true;
                    break;
                }
            }
            if (!nameMatch) {
                continue;
            }
            // check executable path, everything is OK except the user writable path
            std::vector<std::string> allowedPathPrefixes = {
                    "/system/",
                    "/system_ext/",
                    "/vendor/",
                    "/oem/",
                    "/odm/",
                    "/dev/", // wtf?
                    "/product/",
                    "/apex/",
                    "/bin/",
                    "/xbin/",
                    "/sbin/"
            };
            bool pathMatch = false;
            for (const auto &prefix: allowedPathPrefixes) {
                if (p.exe.find(prefix) == 0) {
                    pathMatch = true;
                    break;
                }
            }
            if (!pathMatch) {
                continue;
            }
            // found hal driver service
            status.valid = true;
            status.serviceName = p.name;
            status.serviceProcessId = p.pid;
            status.serviceExecPath = p.exe;
            status.devicePath = DEV_PATH;
            if (auto sp = sWpInstance.lock(); sp) {
                IBaseService *pService = sp.get();
                const QtiEsePmHandler *h;
                if (pService != nullptr && (h = dynamic_cast<QtiEsePmHandler *>(pService)) != nullptr
                    && h->isConnected()) {
                    status.currentAdapter = sp;
                }
            }
            return status;
        }
    }
    // no service found
    status.valid = false;
    LOGV("No QTI eSE PM service found");
    return status;
}

std::shared_ptr<IBaseService> QtiEsePmHandler::getWeakInstance() {
    return sWpInstance.lock();
}

int QtiEsePmHandler::driverRawIoctl0(uint64_t request, uint64_t arg) {
    return -ENOTSUP;
}

int QtiEsePmHandler::driverRawWrite(const std::vector<uint8_t> &buffer) {
    return -ENOTSUP;
}
