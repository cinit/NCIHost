//
// Created by kinit on 2021-11-18.
//
#include <pthread.h>
#include <unistd.h>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <malloc.h>
#include <cerrno>

#include "inject_io_init.h"
#include "daemon_ipc_struct.h"
#include "ipc_looper.h"
#include "request_handler.h"

using namespace halpatch;

void handleIpcPacketInternal(const void *packet, int length) {
    const auto *packetHeader = static_cast<const HalTrxnPacketHeader *>(packet);
    if (packetHeader->type == TrxnType::GENERIC_REQUEST) {
        const auto *request = reinterpret_cast<const HalRequest *>((const char *) packet + sizeof(HalTrxnPacketHeader));
        if (sizeof(HalTrxnPacketHeader) + sizeof(HalRequest) + request->payloadSize > length) {
            sendResponseError(request->id, HalRequestErrorCode::ERR_BAD_REQUEST,
                              "Invalid request packet length");
            return;
        }
        int result = handleRequestPacket(*request, request->payloadSize == 0 ?
                                                   nullptr : (const char *) request + sizeof(HalRequest));
        if (result < 0) {
            // request not handled, send unknown request error
            sendResponseError(request->id, HalRequestErrorCode::ERR_UNKNOWN_REQUEST,
                              "Unknown request");
        }
    }
}

int sendResponsePacket(const HalResponse &response, const void *payload) {
    if (int fd = getDaemonIpcSocket(); fd > 0) {
        size_t pkLen = sizeof(HalTrxnPacketHeader) + sizeof(HalResponse)
                       + (payload == nullptr ? 0 : response.payloadSize);
        void *buf = malloc(pkLen);
        if (buf == nullptr) {
            return -ENOMEM;
        }
        if (payload != nullptr && response.payloadSize > 0) {
            memcpy(((char *) buf) + sizeof(HalTrxnPacketHeader) + sizeof(HalResponse), payload, response.payloadSize);
        }
        auto *pHeader = static_cast<HalTrxnPacketHeader *>(buf);
        pHeader->type = TrxnType::GENERIC_RESPONSE;
        pHeader->length = int(pkLen);
        auto *pResp = reinterpret_cast<HalResponse *>(((char *) buf) + sizeof(HalTrxnPacketHeader));
        memcpy(pResp, &response, sizeof(HalResponse));
        if (write(fd, buf, pkLen) < 0 && errno == EPIPE) {
            closeDaemonIpcSocket();
            free(buf);
            return -EPIPE;
        }
        free(buf);
        return 0;
    }
    return -EPIPE;
}

int sendResponseError(uint32_t requestId, HalRequestErrorCode errorCode, const char *errMsg) {
    HalResponse resp = {requestId, 0, static_cast<uint32_t>(errorCode), errMsg ? uint32_t(strlen(errMsg)) : 0};
    return sendResponsePacket(resp, errMsg);
}

void runIpcWorkerInternal(void *_i32_fd) {
    int fd = (int) reinterpret_cast<size_t>(_i32_fd);
    void *buffer = malloc(HAL_PACKET_MAX_LENGTH);
    if (buffer == nullptr) {
        return;
    }
    ssize_t length;
    while ((length = read(fd, buffer, HAL_PACKET_MAX_LENGTH)) > 0) {
        handleIpcPacketInternal(buffer, int(length));
    }
    if (length < 0 && errno == EPIPE) {
        closeDaemonIpcSocket();
    }
    free(buffer);
}

int startIpcLooper(int fd) {
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK)) {
        return -errno;
    }
    pthread_t ptid = 0;
    int err = pthread_create(&ptid, nullptr,
                             reinterpret_cast<void *(*)(void *)>(&runIpcWorkerInternal), reinterpret_cast<void *>(fd));
    if (err != 0) {
        return err;
    }
    return 0;
}
