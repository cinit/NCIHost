//
// Created by kinit on 2021-11-17.
//
#include <string>

#include "libnxphalpatch/ipc/ipc_requests.h"
#include "libbasehalpatch/ipc/daemon_ipc_struct.h"
#include "rpcprotocol/log/Log.h"
#include "rpcprotocol/utils/ProcessUtils.h"
#include "../../../ipc/IpcStateController.h"

#include "NxpHalHandler.h"

using namespace hwhal;
using namespace halpatch;
using namespace halpatch::nxphal;
using namespace ipcprotocol;

static const char *const LOG_TAG = "NxpHalHandler";

std::weak_ptr<IBaseService> NxpHalHandler::sWpInstance;

std::string_view NxpHalHandler::getName() const noexcept {
    return "NxpHalHandler";
}

std::string_view NxpHalHandler::getDescription() const noexcept {
    return mDescription;
}

int NxpHalHandler::doOnStart(void *args, const std::shared_ptr<IBaseService> &sp) {
    mDescription = "NxpHalHandler@fd" + std::to_string(getFd()) + "-pid-" + std::to_string(getRemotePid());
    sWpInstance = sp;
    return 0;
}

bool NxpHalHandler::doOnStop() {
    return true;
}

void NxpHalHandler::dispatchHwHalIoEvent(const IoOperationEvent &event, const void *payload) {
    std::vector<uint8_t> payloadVec;
    if (payload != nullptr && event.info.bufferLength > 0) {
        payloadVec.assign(static_cast<const uint8_t *>(payload),
                          static_cast<const uint8_t *>(payload) + event.info.bufferLength);
    }
    IpcStateController::getInstance().getNciClientProxy().onIoEvent(event, payloadVec);
    // add the event to the queue tail
    std::scoped_lock lock(mEventMutex);
    mHistoryIoEvents.emplace_back(event, payloadVec);
    // drop the oldest event if the queue is full
    if (mHistoryIoEvents.size() > MAX_HISTORY_EVENT_SIZE) {
        mHistoryIoEvents.pop_front();
    }
}

void NxpHalHandler::dispatchRemoteProcessDeathEvent() {
    LOGW("Remote process death event received, pid: %d", getRemotePid());
    IpcStateController::getInstance().getNciClientProxy().onRemoteDeath(getRemotePid());
    // clear the IO event history if the remote process is dead
    std::scoped_lock lock(mEventMutex);
    mHistoryIoEvents.clear();
}

int NxpHalHandler::getRemotePltHookStatus() {
    HalRequest request = {};
    request.requestCode = static_cast<uint32_t>(RequestId::GET_HOOK_STATUS);
    auto[rc, response, payload] = sendHalRequest(request, nullptr, 1000);
    if (rc < 0) {
        LOGE("Failed to send request, rc=%d", rc);
        return rc;
    }
    return int(response.result);
}

int NxpHalHandler::initRemotePltHook(const OriginHookProcedure &hookProc) {
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

HwServiceStatus NxpHalHandler::getHwServiceStatus() {
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
        if (!p.name.empty() && p.uid > 1000 && p.uid < 10000) {
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
                const NxpHalHandler *h;
                if (pService != nullptr && (h = dynamic_cast<NxpHalHandler *>(pService)) != nullptr
                    && h->isConnected()) {
                    status.currentAdapter = sp;
                }
            }
            LOGV("Found hal driver service, pid=%d, exe=%s, connected=%d", p.pid, p.exe.c_str(),
                 !status.currentAdapter.expired());
            return status;
        }
    }
    // no service found
    status.valid = false;
    return status;
}

std::shared_ptr<IBaseService> NxpHalHandler::getWeakInstance() {
    return sWpInstance.lock();
}

INciHostDaemon::HistoryIoOperationEventList NxpHalHandler::getHistoryIoOperationEvents(uint32_t start, uint32_t count) {
    INciHostDaemon::HistoryIoOperationEventList result;
    std::scoped_lock lock(mEventMutex);
    if (mHistoryIoEvents.empty()) {
        result.totalStartIndex = 0;
        result.totalCount = 0;
        return result;
    }
    // history event list is not empty
    result.totalStartIndex = std::get<0>(mHistoryIoEvents.front()).sequence;
    result.totalCount = uint32_t(mHistoryIoEvents.size());
    // iterate the history events
    auto it = mHistoryIoEvents.cbegin();
    while (it != mHistoryIoEvents.cend()) {
        auto &[event, payload] = *it;
        if (event.sequence >= start) {
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
    return result;
}

void NxpHalHandler::clearHistoryIoOperationEvents() {
    std::scoped_lock lock(mEventMutex);
    mHistoryIoEvents.clear();
}

int NxpHalHandler::driverRawWrite(const std::vector<uint8_t> &buffer) {
    HalRequest request = {};
    request.requestCode = static_cast<uint32_t>(RequestId::DEVICE_OPERATION_WRITE);
    std::vector<uint8_t> requestPayload;
    requestPayload.resize(4 + buffer.size());
    *reinterpret_cast<uint32_t *>(&requestPayload[0]) = uint32_t(buffer.size());
    memcpy(&requestPayload[4], buffer.data(), buffer.size());
    request.payloadSize = uint32_t(requestPayload.size());
    auto[rc, response, payload] = sendHalRequest(request, requestPayload.data(), 1000);
    if (rc < 0) {
        LOGE("Failed to send request, rc=%d", rc);
        return -std::abs(rc);
    }
    return int(response.result);
}

int NxpHalHandler::driverRawIoctl0(uint64_t request, uint64_t arg) {
    HalRequest requestPacket = {};
    requestPacket.requestCode = static_cast<uint32_t>(RequestId::DEVICE_OPERATION_IOCTL0);
    DeviceIoctl0Request requestPayload = {};
    requestPayload.request = request;
    requestPayload.arg = arg;
    requestPacket.payloadSize = uint32_t(sizeof(DeviceIoctl0Request));
    auto[rc, response, payload] = sendHalRequest(requestPacket, &requestPayload, 1000);
    if (rc < 0) {
        LOGE("Failed to send request, rc=%d", rc);
        return -std::abs(rc);
    }
    return int(response.result);
}
