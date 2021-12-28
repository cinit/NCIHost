//
// Created by kinit on 2021-11-18.
//
#include <cstring>
#include <unistd.h>

#include "ipc_requests.h"
#include "../../libbasehalpatch/ipc/request_handler.h"
#include "../../libbasehalpatch/hook/hook_proc_symbols.h"

using namespace halpatch;
using namespace halpatch::qtiesepm;

static volatile bool sInitialized = false;
static volatile int sNciDeviceFd = -1;

const char *const NCI_DEVICE_NAME = "/dev/nq-nci";

namespace halpatchhook::callback {

void afterHook_read(ssize_t result, int fd, const void *buffer, size_t size) {}

void afterHook_write(ssize_t result, int fd, const void *buffer, size_t size) {}

void afterHook_open(int result, const char *name, int flags, uint32_t mode) {
    if (result > 0 && name != nullptr && strncmp(NCI_DEVICE_NAME, name, strlen(NCI_DEVICE_NAME)) == 0) {
        sNciDeviceFd = result;
    }
}

void afterHook_close(int result, int fd) {
    if (fd == sNciDeviceFd) {
        sNciDeviceFd = -1;
    }
}

void afterHook_ioctl(int result, int fd, unsigned long int request, uint64_t arg) {}

void afterHook_select(int result, int nfds, void *readfds, void *writefds, void *exceptfds, void *timeout) {}

}

void handleGetVersionRequest(uint32_t requestId, const void *, uint32_t) {
    const char *version = NCI_HOST_VERSION;
    HalResponse response = {requestId, 0, 0, static_cast<uint32_t>(strlen(version))};
    sendResponsePacket(response, version);
}

void handleGetHookStatusRequest(uint32_t requestId, const void *payload, uint32_t payloadSize) {
    HalResponse response = {requestId, sInitialized, 0, 0};
    sendResponsePacket(response, nullptr);
}

void handleGetNciFdRequest(uint32_t requestId, const void *payload, uint32_t payloadSize) {
    HalResponse response = {requestId, uint32_t(sNciDeviceFd), 0, 0};
    sendResponsePacket(response, nullptr);
}

void handleInitPltHookRequest(uint32_t requestId, const void *payload, uint32_t payloadSize) {
    HalResponse response = {requestId, 0, 0, 0};
    const auto *originHookProcedure = reinterpret_cast<const OriginHookProcedure *>(payload);
    if (payloadSize != sizeof(OriginHookProcedure) || originHookProcedure == nullptr
        || originHookProcedure->struct_size != sizeof(OriginHookProcedure)) {
        sendResponseError(requestId, HalRequestErrorCode::ERR_INVALID_ARGUMENT,
                          "invalid payload OriginHookProcedure");
    } else {
        int result = hook_sym_init(originHookProcedure);
        if (result == 0) {
            sInitialized = true;
        }
        response.result = result;
        sendResponsePacket(response, nullptr);
    }
}

int handleRequestPacket(const HalRequest &request, const void *payload) {
    uint32_t requestId = request.id;
    auto requestCode = static_cast<RequestId>(request.requestCode);
    uint32_t payloadSize = request.payloadSize;
    switch (requestCode) {
        case RequestId::GET_VERSION:
            handleGetVersionRequest(requestId, payload, payloadSize);
            return 0;
        case RequestId::GET_HOOK_STATUS:
            handleGetHookStatusRequest(requestId, payload, payloadSize);
            return 0;
        case RequestId::INIT_PLT_HOOK:
            handleInitPltHookRequest(requestId, payload, payloadSize);
            return 0;
        case RequestId::GET_NCI_FD:
            handleGetNciFdRequest(requestId, payload, payloadSize);
            return 0;
        default:
            return -1;
    }
}
