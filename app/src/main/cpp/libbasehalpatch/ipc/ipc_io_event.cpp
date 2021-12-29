// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-10-21.
//

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <ctime>

#include "ipc_io_event.h"
#include "inject_io_init.h"

using namespace halpatch;

static uint32_t g_IoEventSequence = 0;

static uint64_t getCurrentTimeMillis() {
    struct timespec ts = {};
    clock_gettime(CLOCK_REALTIME, &ts);
    return uint64_t(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
}

static void postIoEvent(const IoSyscallInfo &opInfo, const void *ioBuffer, size_t ioBufSize) {
    if (int fd = getDaemonIpcSocket(); fd > 0) {
        size_t pkLen = sizeof(HalTrxnPacketHeader) + sizeof(IoSyscallEvent) + (ioBuffer == nullptr ? 0 : ioBufSize);
        void *buf = malloc(pkLen);
        if (buf == nullptr) {
            return;
        }
        if (ioBuffer != nullptr && ioBufSize > 0) {
            memcpy(((char *) buf) + sizeof(HalTrxnPacketHeader) + sizeof(IoSyscallEvent), ioBuffer, ioBufSize);
        }
        auto *pHeader = static_cast<HalTrxnPacketHeader *>(buf);
        pHeader->type = TrxnType::IO_EVENT;
        pHeader->length = int(pkLen);
        auto *pEvent = reinterpret_cast<IoSyscallEvent *>(((char *) buf) + sizeof(HalTrxnPacketHeader));
        pEvent->rfu = 0;
        // we don't know the uniqueSequence and sourceType, leave them as 0, they will be filled in by the daemon
        pEvent->uniqueSequence = 0;
        pEvent->sourceType = 0;
        pEvent->sourceSequence = g_IoEventSequence++;
        pEvent->timestamp = getCurrentTimeMillis();
        pEvent->info = opInfo;
        pEvent->info.bufferLength = int64_t(ioBufSize);
        if (write(fd, buf, pkLen) < 0 && errno == EPIPE) {
            closeDaemonIpcSocket();
        }
        free(buf);
    }
}

void invokeReadResultCallback(ssize_t result, int fd, const void *buffer, size_t size) {
    IoSyscallInfo info = {static_cast<int32_t>(OpType::OP_TYPE_IO_READ),
                          fd, result, (uint64_t) buffer, uint64_t(size), 0};
    halpatchhook::callback::afterHook_read(result, fd, buffer, size);
    if (result < 0) {
        postIoEvent(info, nullptr, 0);
    } else {
        postIoEvent(info, buffer, size);
    }
}

void invokeWriteResultCallback(ssize_t result, int fd, const void *buffer, size_t size) {
    IoSyscallInfo info = {static_cast<int32_t>(OpType::OP_TYPE_IO_WRITE),
                          fd, result, (uint64_t) buffer, uint64_t(size), 0};
    halpatchhook::callback::afterHook_write(result, fd, buffer, size);
    if (result < 0) {
        postIoEvent(info, nullptr, 0);
    } else {
        postIoEvent(info, buffer, size);
    }
}

void invokeOpenResultCallback(int result, const char *name, int flags, uint32_t mode) {
    IoSyscallInfo info = {static_cast<int32_t>(OpType::OP_TYPE_IO_OPEN),
                          result < 0 ? -1 : result, result, static_cast<uint64_t>(flags), mode, 0};
    halpatchhook::callback::afterHook_open(result, name, flags, mode);
    if (name != nullptr) {
        postIoEvent(info, name, strlen(name));
    } else {
        postIoEvent(info, nullptr, 0);
    }
}

void invokeCloseResultCallback(int result, int fd) {
    IoSyscallInfo info = {static_cast<int32_t>(OpType::OP_TYPE_IO_CLOSE),
                          fd, result, 0, 0, 0};
    halpatchhook::callback::afterHook_close(result, fd);
    postIoEvent(info, nullptr, 0);
}

void invokeIoctlResultCallback(int result, int fd, unsigned long int request, uint64_t arg) {
    IoSyscallInfo info = {static_cast<int32_t>(OpType::OP_TYPE_IO_IOCTL),
                          fd, result, request, arg, 0};
    halpatchhook::callback::afterHook_ioctl(result, fd, request, arg);
    postIoEvent(info, nullptr, 0);
}

void invokeSelectResultCallback(int result, int nfds, void *readfds, void *writefds, void *exceptfds, void *timeout) {
    IoSyscallInfo info = {static_cast<int32_t>(OpType::OP_TYPE_IO_SELECT),
                          0, result, 0, 0, 0};
    halpatchhook::callback::afterHook_select(result, nfds, readfds, writefds, exceptfds, timeout);
    postIoEvent(info, nullptr, 0);
}
