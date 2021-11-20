//
// Created by kinit on 2021-11-17.
//
#include <string>

#include "libbasehalpatch/ipc/daemon_ipc_struct.h"
#include "rpcprotocol/log/Log.h"

#include "NxpHalHandler.h"

using namespace hwhal;

static const char *const LOG_TAG = "NxpHalHandler";

std::string_view NxpHalHandler::getName() const noexcept {
    return "NxpHalHandler";
}

std::string_view NxpHalHandler::getDescription() const noexcept {
    return mDescription;
}

void NxpHalHandler::dispatchRemoteProcessDeathEvent() {
    LOGI("Remote process death event received, pid: %d", getRemotePid());
}

int NxpHalHandler::doOnStart(void *args) {
    mDescription = "NxpHalHandler@fd" + std::to_string(getFd()) + "-pid-" + std::to_string(getRemotePid());
    return 0;
}

bool NxpHalHandler::doOnStop() {
    return true;
}

void NxpHalHandler::dispatchHwHalIoEvent(const IoOperationEvent &event, const void *payload) {
    LOGI("HwHalIoEvent %d received, op=%d, fd=%u, arg1=%ld, len=%ld",
         event.sequence, event.info.opType, event.info.fd, event.info.directArg1, event.info.bufferLength);
}
