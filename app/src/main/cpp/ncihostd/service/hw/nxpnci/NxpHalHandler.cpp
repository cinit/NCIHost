//
// Created by kinit on 2021-11-17.
//
#include <string>

#include "libnxphalpatch/ipc/ipc_requests.h"
#include "libbasehalpatch/ipc/daemon_ipc_struct.h"
#include "rpcprotocol/log/Log.h"
#include "../../../ipc/IpcStateController.h"

#include "NxpHalHandler.h"

using namespace hwhal;
using namespace ipcprotocol;

static const char *const LOG_TAG = "NxpHalHandler";

std::string_view NxpHalHandler::getName() const noexcept {
    return "NxpHalHandler";
}

std::string_view NxpHalHandler::getDescription() const noexcept {
    return mDescription;
}

int NxpHalHandler::doOnStart(void *args) {
    mDescription = "NxpHalHandler@fd" + std::to_string(getFd()) + "-pid-" + std::to_string(getRemotePid());
    return 0;
}

bool NxpHalHandler::doOnStop() {
    return true;
}

void NxpHalHandler::dispatchHwHalIoEvent(const IoOperationEvent &event, const void *payload) {
    IpcStateController::getInstance().getNciHostDaemon().sendIoOperationEvent(event, payload);
}

void NxpHalHandler::dispatchRemoteProcessDeathEvent() {
    LOGW("Remote process death event received, pid: %d", getRemotePid());
    IpcStateController::getInstance().getNciHostDaemon().sendRemoteDeathEvent(getRemotePid());
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
