//
// Created by kinit on 2021-12-21.
//

#include <unistd.h>

#include "rpcprotocol/log/Log.h"

#include "BaseRemoteAndroidService.h"

using namespace androidsvc;

constexpr int MAX_PACKET_SIZE = 65536;
const char *const LOG_TAG = "BaseRemoteAndroidService";

std::vector<uint8_t> BaseRemoteAndroidService::RequestPacket::toByteArray() const {
    const uint32_t type = BaseRemoteAndroidService::TYPE_REQUEST;
    std::vector<uint8_t> data;
    data.reserve(sizeof(uint32_t) * 4 + payload.size());
    data.insert(data.end(), (uint8_t *) &type, (uint8_t *) &type + sizeof(uint32_t));
    data.insert(data.end(), (uint8_t *) &sequence, (uint8_t *) &sequence + sizeof(uint32_t));
    data.insert(data.end(), (uint8_t *) &requestCode, (uint8_t *) &requestCode + sizeof(uint32_t));
    data.insert(data.end(), (uint8_t *) &arg0, (uint8_t *) &arg0 + sizeof(uint32_t));
    if (!payload.empty()) {
        data.insert(data.end(), payload.begin(), payload.end());
    }
    return data;
}

std::vector<uint8_t> BaseRemoteAndroidService::RequestPacket::toByteArray(uint32_t overrideSequence) const {
    const uint32_t type = BaseRemoteAndroidService::TYPE_REQUEST;
    std::vector<uint8_t> data;
    data.reserve(sizeof(uint32_t) * 4 + payload.size());
    data.insert(data.end(), (uint8_t *) &type, (uint8_t *) &type + sizeof(uint32_t));
    data.insert(data.end(), (uint8_t *) &overrideSequence, (uint8_t *) &overrideSequence + sizeof(uint32_t));
    data.insert(data.end(), (uint8_t *) &requestCode, (uint8_t *) &requestCode + sizeof(uint32_t));
    data.insert(data.end(), (uint8_t *) &arg0, (uint8_t *) &arg0 + sizeof(uint32_t));
    if (!payload.empty()) {
        data.insert(data.end(), payload.begin(), payload.end());
    }
    return data;
}

std::vector<uint8_t> BaseRemoteAndroidService::ResponsePacket::toByteArray() const {
    const uint32_t type = BaseRemoteAndroidService::TYPE_RESPONSE;
    std::vector<uint8_t> data;
    data.reserve(sizeof(uint32_t) * 4 + payload.size());
    data.insert(data.end(), (uint8_t *) &type, (uint8_t *) &type + sizeof(uint32_t));
    data.insert(data.end(), (uint8_t *) &sequence, (uint8_t *) &sequence + sizeof(uint32_t));
    data.insert(data.end(), (uint8_t *) &resultCode, (uint8_t *) &resultCode + sizeof(uint32_t));
    data.insert(data.end(), (uint8_t *) &errorCode, (uint8_t *) &errorCode + sizeof(uint32_t));
    if (!payload.empty()) {
        data.insert(data.end(), payload.begin(), payload.end());
    }
    return data;
}

std::vector<uint8_t> BaseRemoteAndroidService::EventPacket::toByteArray() const {
    const uint32_t type = BaseRemoteAndroidService::TYPE_EVENT;
    std::vector<uint8_t> data;
    data.reserve(sizeof(uint32_t) * 4 + payload.size());
    data.insert(data.end(), (uint8_t *) &type, (uint8_t *) &type + sizeof(uint32_t));
    data.insert(data.end(), (uint8_t *) &sequence, (uint8_t *) &sequence + sizeof(uint32_t));
    data.insert(data.end(), (uint8_t *) &eventCode, (uint8_t *) &eventCode + sizeof(uint32_t));
    data.insert(data.end(), (uint8_t *) &arg0, (uint8_t *) &arg0 + sizeof(uint32_t));
    if (!payload.empty()) {
        data.insert(data.end(), payload.begin(), payload.end());
    }
    return data;
}

std::tuple<int, BaseRemoteAndroidService::ResponsePacket>
BaseRemoteAndroidService::sendRequest(const BaseRemoteAndroidService::RequestPacket &request, int timeout_ms) {
    uint32_t seq = mSequenceId++;
    std::vector<uint8_t> data = request.toByteArray(seq);
    auto packetLength = uint32_t(data.size());
    std::vector<uint8_t> packetWithLength;
    packetWithLength.reserve(sizeof(uint32_t) + data.size());
    packetWithLength.insert(packetWithLength.end(), (uint8_t *) &packetLength,
                            (uint8_t *) &packetLength + sizeof(uint32_t));
    packetWithLength.insert(packetWithLength.end(), data.begin(), data.end());
    std::condition_variable condvar;
    ResponsePacket response;
    LpcReturnStatus returnStatus = {&condvar, &response, 0};
    {
        std::unique_lock lk(mIpcMutex);
        mWaitingRequests.put(seq, &returnStatus);
        ssize_t ret = write(mFd, packetWithLength.data(), packetWithLength.size());
        if (ret > 0) {
            auto waitResult = condvar.wait_for(lk, std::chrono::milliseconds(timeout_ms));
            if (waitResult == std::cv_status::timeout) {
                LOGE("sendRequest timeout");
                mWaitingRequests.remove(seq);
                return std::make_tuple(ETIMEDOUT, ResponsePacket{});
            } else {
                mWaitingRequests.remove(seq);
                // handle response
                const auto *pResponse = returnStatus.response;
                if (pResponse->sequence != seq) {
                    LOGE("sendRequest: invalid response id %d, expected %d", pResponse->sequence, seq);
                    return std::make_tuple(EINVAL, ResponsePacket{});
                } else {
                    return std::make_tuple(0, *pResponse);
                }
            }
        } else {
            int err = errno;
            LOGE("write() failed with %d", err);
            mWaitingRequests.remove(seq);
            return std::make_tuple(err, ResponsePacket{});
        }
    }
}

static int readFully(int fd, void *buffer, size_t size) {
    size_t remaining = size;
    while (remaining > 0) {
        ssize_t n = read(fd, buffer, remaining);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -errno;
        }
        if (n == 0) {
            return -EIO;
        }
        remaining -= n;
        buffer = (uint8_t *) buffer + n;
    }
    return 0;
}

void BaseRemoteAndroidService::workerThread(BaseRemoteAndroidService *self) {
    std::vector<uint8_t> buffer(MAX_PACKET_SIZE);
    int fd = self->mFd;
    while (true) {
        uint32_t packetLength = 0;
        if (int err = readFully(fd, &packetLength, sizeof(uint32_t));err < 0) {
            LOGE("readFully() failed with %d", err);
            break;
        }
        if (packetLength < 16 || packetLength > MAX_PACKET_SIZE) {
            LOGE("invalid packet length %d", packetLength);
            // stream corrupted, close connection
            break;
        }
        if (int err = readFully(fd, buffer.data(), packetLength); err < 0) {
            LOGE("readFully() failed with %d", err);
            break;
        }
        self->dispatchServiceIpcPacket(buffer.data(), packetLength);
    }
    close(fd);
    self->mFd = -1;
    self->mThread = 0;
    self->dispatchConnectionBroken();
}

void BaseRemoteAndroidService::dispatchServiceIpcPacket(const void *packet, size_t length) {
    const auto *pU32 = reinterpret_cast<const uint32_t *>(packet);
    uint32_t type = pU32[0];
    uint32_t v1 = pU32[1];
    uint32_t v2 = pU32[2];
    uint32_t v3 = pU32[3];
    switch (type) {
        case TYPE_EVENT: {
            EventPacket event;
            event.sequence = v1;
            event.eventCode = v2;
            event.arg0 = v3;
            event.payload.resize(length - 16);
            event.payload.assign(reinterpret_cast<const uint8_t *>(packet) + 16,
                                 reinterpret_cast<const uint8_t *>(packet) + length);
            dispatchServiceEvent(event);
            break;
        }
        case TYPE_RESPONSE: {
            uint32_t seq = v1;
            std::unique_lock lock(mIpcMutex);
            if (LpcReturnStatus **it = mWaitingRequests.get(seq); it != nullptr && *it != nullptr) {
                mWaitingRequests.remove(seq);
                // notify the waiting thread
                LpcReturnStatus *status = *it;
                status->response->sequence = seq;
                status->response->resultCode = v2;
                status->response->errorCode = v3;
                status->response->payload.resize(length - 16);
                status->response->payload.assign(reinterpret_cast<const uint8_t *>(packet) + 16,
                                                 reinterpret_cast<const uint8_t *>(packet) + length);
                status->error = 0;
                status->cond->notify_all();
            } else {
                LOGE("No request found for response %d", seq);
            }
            break;
        }
        default:
            LOGE("Unknown packet type %d", type);
    }
}

int BaseRemoteAndroidService::startWorkerThread(int streamSocketFd) {
    mFd = streamSocketFd;
    int err = pthread_create(&mThread, nullptr, reinterpret_cast<void *(*)(void *)>(&workerThread), this);
    if (err != 0) {
        LOGE("pthread_create failed, errno: %d", err);
        mThread = 0;
        return err;
    }
    return 0;
}
