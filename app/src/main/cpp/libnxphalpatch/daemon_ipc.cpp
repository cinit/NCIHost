// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-10-21.
//

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>

#include "daemon_ipc.h"
#include "inject_io_init.h"

static void postIoEvent(IoOperationInfo *opInfo, const void *ioBuffer, size_t bufLen) {
    int fd = getDaemonIpcSocket();
    if (fd > 0) {
        if (ioBuffer != nullptr && bufLen > 0) {
            size_t len = sizeof(IoOperationInfo) + bufLen;
            void *buf = malloc(len);
            if (buf != nullptr) {
                memcpy(((char *) buf) + sizeof(IoOperationInfo), ioBuffer, bufLen);
                auto *p = static_cast<IoOperationInfo *>(buf);
                p->bufferLength = int64_t(bufLen);
                if (write(fd, buf, sizeof(IoOperationInfo) + bufLen) < 0 && errno == EPIPE) {
                    closeDaemonIpcSocket();
                }
                free(buf);
            }
        } else {
            opInfo->bufferLength = 0;
            if (write(fd, opInfo, sizeof(IoOperationInfo)) < 0 && errno == EPIPE) {
                closeDaemonIpcSocket();
            }
        }
    }
}

void invokeReadResultCallback(ssize_t result, int fd, const void *buffer, size_t size) {
    IoOperationInfo info = {static_cast<int32_t>(OpType::OP_TYPE_IO_READ),
                            fd, result, (uint64_t) buffer, uint64_t(size), 0};
    if (result < 0) {
        postIoEvent(&info, nullptr, 0);
    } else {
        postIoEvent(&info, buffer, size);
    }
}

void invokeWriteResultCallback(ssize_t result, int fd, const void *buffer, size_t size) {
    IoOperationInfo info = {static_cast<int32_t>(OpType::OP_TYPE_IO_WRITE),
                            fd, result, (uint64_t) buffer, uint64_t(size), 0};
    if (result < 0) {
        postIoEvent(&info, nullptr, 0);
    } else {
        postIoEvent(&info, buffer, size);
    }
}

void invokeOpenResultCallback(int result, const char *name, int flags, uint32_t mode) {
    IoOperationInfo info = {static_cast<int32_t>(OpType::OP_TYPE_IO_OPEN),
                            result < 0 ? -1 : result, result, static_cast<uint64_t>(flags), mode, 0};
    if (name != nullptr) {
        postIoEvent(&info, name, strlen(name));
    } else {
        postIoEvent(&info, nullptr, 0);
    }
}

void invokeCloseResultCallback(int result, int fd) {
    IoOperationInfo info = {static_cast<int32_t>(OpType::OP_TYPE_IO_CLOSE),
                            fd, result, 0, 0, 0};
    postIoEvent(&info, nullptr, 0);
}

void invokeIoctlResultCallback(int result, int fd, unsigned long int request, uint64_t arg) {
    IoOperationInfo info = {static_cast<int32_t>(OpType::OP_TYPE_IO_IOCTL),
                            fd, result, request, arg, 0};
    postIoEvent(&info, nullptr, 0);
}

void invokeSelectResultCallback(int result) {
    IoOperationInfo info = {static_cast<int32_t>(OpType::OP_TYPE_IO_SELECT),
                            0, result, 0, 0, 0};
    postIoEvent(&info, nullptr, 0);
}
