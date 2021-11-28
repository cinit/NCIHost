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
}

void NxpHalHandler::dispatchRemoteProcessDeathEvent() {
    LOGW("Remote process death event received, pid: %d", getRemotePid());
    IpcStateController::getInstance().getNciClientProxy().onRemoteDeath(getRemotePid());
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
        if (p.name == EXEC_NAME && p.uid < 10000) {
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
