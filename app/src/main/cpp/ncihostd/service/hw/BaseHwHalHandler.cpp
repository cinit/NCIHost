//
// Created by kinit on 2021-11-17.
//
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

#include "libbasehalpatch/ipc/daemon_ipc_struct.h"
#include "rpcprotocol/log/Log.h"
#include "../ServiceManager.h"

#include "BaseHwHalHandler.h"

using namespace hwhal;

static const char *const LOG_TAG = "BaseHwHalHandler";

void BaseHwHalHandler::stopSelf() {
    ServiceManager::getInstance().stopService(this);
}

int BaseHwHalHandler::onStart(void *args) {
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
    int result = doOnStart(args);
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
    return true;
}

void BaseHwHalHandler::workerThread(BaseHwHalHandler *self) {
    ssize_t i;
    void *buffer = malloc(HAL_PACKET_MAX_LENGTH);
    if (buffer == nullptr) {
        self->mIsRunning = false;
        return;
    }
    while (true) {
        i = read(self->mFd, buffer, HAL_PACKET_MAX_LENGTH);
        if (i > 0) {
            self->dispatchHwHalPatchIpcPacket(buffer, i);
        } else {
            int err = errno;
            if (err == EPIPE) {
                // remote process has closed the connection,
                // is the remote process still alive?
                if (access(("/proc/" + std::to_string(self->mRemotePid)).c_str(), F_OK) != 0) {
                    // remote process is dead, notify the user
                    self->dispatchRemoteProcessDeathEvent();
                } else {
                    // remote process is still alive, but why?
                    // we don't know, but we can't do anything about it
                    LOGE("read() failed with EPIPE, but remote process is still alive");
                }
                close(self->mFd);
                break;
            } else if (err == EINTR) {
                continue;
            } else {
                LOGE("read() failed with %d", err);
                break;
            }
        }
    }
    free(buffer);
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
            const auto *event = reinterpret_cast<const IoOperationEvent *>(
                    (const char *) packetBuffer + sizeof(HalTrxnPacketHeader));
            const void *payload = nullptr;
            if (event->info.bufferLength != 0) {
                payload = (const char *) packetBuffer + sizeof(HalTrxnPacketHeader) + sizeof(IoOperationEvent);
            }
            dispatchHwHalIoEvent(*event, payload);
        }
        default:
            LOGE("Unknown packet type %d", header->type);
    }
}
