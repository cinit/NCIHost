//
// Created by kinit on 2021-11-17.
//
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>

#include "libbasehalpatch/ipc/daemon_ipc_struct.h"
#include "rpcprotocol/log/Log.h"
#include "../ServiceManager.h"

#include "BaseHwHalHandler.h"

using namespace hwhal;
using namespace halpatch;
using std::vector, std::tuple;

static const char *const LOG_TAG = "BaseHwHalHandler";

void BaseHwHalHandler::stopSelf() {
    ServiceManager::getInstance().stopService(this);
}

int BaseHwHalHandler::onStart(void *args, const std::shared_ptr<IBaseService> &sp) {
    StartupInfo const *startupInfo = static_cast<StartupInfo *>(args);
    if (startupInfo == nullptr || startupInfo->fd < 0) {
        return -EINVAL;
    }
    int fd = startupInfo->fd;
    int pid = startupInfo->pid;
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK) < 0) {
        return -errno;
    }
    if (access(("/proc/" + std::to_string(pid)).c_str(), F_OK) != 0) {
        return (errno == ENOENT) ? -ESRCH : -errno;
    }
    mFd = fd;
    mRemotePid = pid;
    int result = doOnStart(args, sp);
    if (result < 0) {
        return result;
    }
    int err = pthread_create(&mThread, nullptr, (void *(*)(void *)) &BaseHwHalHandler::workerThread, this);
    if (err != 0) {
        (void) doOnStop();
        return -err;
    }
    mIsRunning = true;
    return result;
}

bool BaseHwHalHandler::onStop() {
    if (mIsRunning) {
        return false;
    }
    if (!doOnStop()) {
        return false;
    }
    // do not wait for thread exit here, std::shared_ptr will keep the object alive
    return true;
}

void BaseHwHalHandler::workerThread(BaseHwHalHandler *self) {
    std::shared_ptr<IBaseService> spSelf;
    for (const auto &wp: ServiceManager::getInstance().getRunningServices()) {
        if (const auto &sp = wp.lock()) {
            if (static_cast<IBaseService *>(self) == sp.get()) {
                spSelf = sp;
                break;
            }
        }
    }
    if (!spSelf) {
        LOGE("unable to find self in running services, this may cause bugs, eg access to a deleted object");
    }
    ssize_t i;
    std::vector<uint8_t> buffer(HAL_PACKET_MAX_LENGTH);
    while (true) {
        errno = 0; // just in case
        i = read(self->mFd, buffer.data(), HAL_PACKET_MAX_LENGTH);
        if (i > 0) {
            self->dispatchHwHalPatchIpcPacket(buffer.data(), i);
        } else {
            int err = errno;
            if (err == EPIPE || err == 0) {
                // remote process has closed the connection,
                // is the remote process still alive?
                if (access(("/proc/" + std::to_string(self->mRemotePid)).c_str(), F_OK) != 0) {
                    LOGW("remote process %d has died", self->mRemotePid);
                    // remote process is dead, notify the user
                    self->dispatchRemoteProcessDeathEvent();
                } else {
                    // remote process is still alive, but why?
                    // we don't know, but we can't do anything about it
                    LOGE("read() failed with EPIPE, but remote process is still alive");
                }
                close(self->mFd);
                self->mFd = -1;
                break;
            } else if (err == EINTR) {
                continue;
            } else {
                LOGE("read() failed with %d", err);
                close(self->mFd);
                self->mFd = -1;
                break;
            }
        }
    }
    self->mIsRunning = false;
    self->stopSelf();
}

void BaseHwHalHandler::dispatchHwHalPatchIpcPacket(const void *packetBuffer, size_t length) {
    const auto *header = (HalTrxnPacketHeader *) packetBuffer;
    if (header->length > length) {
        LOGE("Invalid packet length %d/%zu", header->length, length);
        return;
    }
    switch (header->type) {
        case TrxnType::IO_EVENT: {
            const auto *event = reinterpret_cast<const IoSyscallEvent *>(
                    (const char *) packetBuffer + sizeof(HalTrxnPacketHeader));
            const void *payload = nullptr;
            if (event->info.bufferLength != 0) {
                payload = (const char *) packetBuffer + sizeof(HalTrxnPacketHeader) + sizeof(IoSyscallEvent);
            }
            dispatchHwHalIoEvent(*event, payload);
            break;
        }
        case TrxnType::GENERIC_RESPONSE: {
            const auto *response = reinterpret_cast<const HalResponse *>(
                    (const char *) packetBuffer + sizeof(HalTrxnPacketHeader));
            uint32_t seq = response->id;
            std::unique_lock lock(mIpcMutex);
            if (LpcReturnStatus **it = mWaitingRequests.get(seq); it != nullptr && *it != nullptr) {
                mWaitingRequests.remove(seq);
                // notify the waiting thread
                LpcReturnStatus *status = *it;
                status->buffer->resize(length);
                memcpy(status->buffer->data(), packetBuffer, length);
                status->error = 0;
                status->cond->notify_all();
            } else {
                LOGE("No request found for response %d", seq);
            }
            break;
        }
        default:
            LOGE("Unknown packet type %d", header->type);
    }
}

tuple<int, HalResponse, vector<uint8_t>> BaseHwHalHandler::sendHalRequest(const HalRequest &request,
                                                                          const void *payload, int timeout_ms) {
    uint32_t seq = mSequenceId++;
    auto packetLength = uint32_t(sizeof(HalTrxnPacketHeader) + sizeof(HalRequest) + request.payloadSize);
    std::vector<uint8_t> packetBuffer;
    packetBuffer.resize(packetLength);
    auto *header = reinterpret_cast<HalTrxnPacketHeader *>(&packetBuffer[0]);
    *header = {.length=packetLength, .type=TrxnType::GENERIC_REQUEST};
    memcpy(&packetBuffer[sizeof(HalTrxnPacketHeader)], &request, sizeof(HalRequest));
    auto *pRequest = reinterpret_cast<HalRequest *>(&packetBuffer[sizeof(HalTrxnPacketHeader)]);
    pRequest->id = seq;
    if (payload != nullptr) {
        memcpy(&packetBuffer[sizeof(HalTrxnPacketHeader) + sizeof(HalRequest)], payload, request.payloadSize);
    }
    std::condition_variable condvar;
    std::vector<uint8_t> responseBuffer;
    LpcReturnStatus returnStatus = {&condvar, &responseBuffer, 0};
    {
        std::unique_lock lk(mIpcMutex);
        mWaitingRequests.put(seq, &returnStatus);
        ssize_t ret = write(mFd, &packetBuffer[0], packetLength);
        if (ret > 0) {
            auto waitResult = condvar.wait_for(lk, std::chrono::milliseconds(timeout_ms));
            if (waitResult == std::cv_status::timeout) {
                LOGE("sendHalRequest timeout");
                mWaitingRequests.remove(seq);
                return std::make_tuple(-ETIMEDOUT, HalResponse{}, vector<uint8_t>());
            } else {
                mWaitingRequests.remove(seq);
                // handle response
                const auto *pResponse = reinterpret_cast<HalResponse *>(&responseBuffer[sizeof(HalTrxnPacketHeader)]);
                if (pResponse->id != seq) {
                    LOGE("sendHalRequest: invalid response id %d, expected %d", pResponse->id, seq);
                    return std::make_tuple(-EINVAL, HalResponse{}, vector<uint8_t>());
                } else {
                    return std::make_tuple(0, *pResponse, vector<uint8_t>(
                            &responseBuffer[sizeof(HalTrxnPacketHeader) + sizeof(HalResponse)],
                            &responseBuffer[sizeof(HalTrxnPacketHeader) + sizeof(HalResponse) +
                                            pResponse->payloadSize]));
                }
            }
        } else {
            int err = errno;
            LOGE("write() failed with %d", err);
            mWaitingRequests.remove(seq);
            return std::make_tuple(err, HalResponse(), vector<uint8_t>());
        }
    }
}

bool BaseHwHalHandler::isConnected() const {
    return mIsRunning;
}
