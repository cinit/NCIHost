//
// Created by kinit on 2021-10-21.
//

#ifndef NCI_HOST_NATIVES_DAEMON_IPC_H
#define NCI_HOST_NATIVES_DAEMON_IPC_H

#include <cstdint>

using InjectInitInfo = struct {
    char daemonVersion[32];
    char socketName[64];
};
static_assert(sizeof(InjectInitInfo) == 32 + 64);

using LogInfo = struct {
    char *msg;
    uint64_t length;
};
static_assert(sizeof(LogInfo) == 16);

enum class OpType : uint32_t {
    OP_TYPE_IO_OPEN = 1,
    OP_TYPE_IO_CLOSE = 2,
    OP_TYPE_IO_READ = 3,
    OP_TYPE_IO_WRITE = 4,
    OP_TYPE_IO_IOCTL = 5,
};

using IoOperationInfo = struct {
    uint32_t hookId;
    uint32_t opType;
    int32_t fd;
    int32_t retErrno;
    int64_t retValue;
    int64_t directArgValue;
    int64_t bufferLength;
    void *buffer;
};
static_assert(sizeof(IoOperationInfo) == 48);

enum class TracerCallId {
    TRACER_CALL_LOG = 1,
    TRACER_CALL_INIT = 2,
    TRACER_CALL_CONNECT = 3,
    TRACER_CALL_DETACH = 0xFF,
};

void *tracer_call(TracerCallId cmd, void *args);

extern "C" void tracer_printf(const char *fmt, ...)
__attribute__ ((__format__ (__printf__, 1, 2)));

void invokeReadResultCallback(ssize_t result, int fd, void *buffer, ssize_t size);

void invokeWriteResultCallback(ssize_t result, int fd, void *buffer, ssize_t size);

void invokeOpenResultCallback(int result, const char *name, int flags, uint32_t mode);

void invokeCloseResultCallback(int result, int fd);

void invokeIoctlResultCallback(int result, int fd, unsigned long int request, uint64_t arg);

void invokeSelectResultCallback(int result);

#endif //NCI_HOST_NATIVES_DAEMON_IPC_H
