//
// Created by kinit on 2021-10-24.
//

#ifndef NCI_HOST_NATIVES_DAEMON_IPC_STRUCT_H
#define NCI_HOST_NATIVES_DAEMON_IPC_STRUCT_H

#ifdef __cplusplus

#include <cstdint>
#include <cstddef>

using InjectInitInfo = struct {
    char daemonVersion[32];
    char socketName[64];
};
static_assert(sizeof(InjectInitInfo) == 32 + 64);

/**
 * uint64_t does not align properly on i386
 */
using LogInfo = struct __attribute__((aligned(8))) {
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
    OP_TYPE_IO_SELECT = 6,
};

using IoOperationInfo = struct {
    int32_t opType;
    int32_t fd;
    int64_t retValue;
    uint64_t directArg1;
    uint64_t directArg2;
    int64_t bufferLength;
};
static_assert(sizeof(IoOperationInfo) == 40);

enum class TracerCallId {
    TRACER_CALL_LOG = 1,
    TRACER_CALL_INIT = 2,
    TRACER_CALL_CONNECT = 3,
    TRACER_CALL_HOOK_SYM = 4,
    TRACER_CALL_DONE = 0x20,
    TRACER_CALL_DETACH = 0xFF,
};

#else
#include <stdint.h>
#endif //__cplusplus

struct OriginHookProcedure {
    uint32_t struct_size;
    uint32_t entry_count;
    uint64_t libnci_base;
    uint32_t off_plt_open_2;
    uint32_t off_plt_open;
    uint32_t off_plt_read_chk;
    uint32_t off_plt_read;
    uint32_t off_plt_write_chk;
    uint32_t off_plt_write;
    uint32_t off_plt_close;
    uint32_t off_plt_ioctl;
    uint32_t off_plt_select;
    uint32_t unused32_0;
};
#ifdef __cplusplus
static_assert(sizeof(OriginHookProcedure) == 56);
#endif

#endif //NCI_HOST_NATIVES_DAEMON_IPC_STRUCT_H
