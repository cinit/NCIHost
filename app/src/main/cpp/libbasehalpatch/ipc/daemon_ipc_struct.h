//
// Created by kinit on 2021-10-24.
//

#ifndef NCI_HOST_NATIVES_DAEMON_IPC_STRUCT_H
#define NCI_HOST_NATIVES_DAEMON_IPC_STRUCT_H

#include <cstdint>
#include <cstddef>

#include "../hook/hook_sym_struct.h"

constexpr int HAL_PACKET_MAX_LENGTH = 16384;

enum class HalRequestErrorCode : uint32_t {
    ERR_SUCCESS = 0,
    ERR_BAD_REQUEST = 0x12,
    ERR_UNKNOWN_REQUEST = 0x13,
    ERR_INVALID_ARGUMENT = 0x15,
    ERR_REMOTE_INTERNAL_ERROR = 0x21,
};

enum class TrxnType : uint32_t {
    GENERIC_EVENT = 2,
    IO_EVENT = 0x40,
    IO_REQUEST = 0x41,
    GENERIC_REQUEST = 0x50,
    GENERIC_RESPONSE = 0x51,
};

struct HalTrxnPacketHeader {
    uint32_t length; // including header
    TrxnType type;
};
static_assert(sizeof(HalTrxnPacketHeader) == 8);

struct HalRequest {
    uint32_t id;
    uint32_t requestCode;
    uint32_t argument;
    uint32_t payloadSize;
};
static_assert(sizeof(HalRequest) == 16);

struct HalResponse {
    uint32_t id;
    uint32_t result;
    uint32_t error;
    uint32_t payloadSize;
};
static_assert(sizeof(HalResponse) == 16);

enum class OpType : uint32_t {
    OP_TYPE_IO_OPEN = 1,
    OP_TYPE_IO_CLOSE = 2,
    OP_TYPE_IO_READ = 3,
    OP_TYPE_IO_WRITE = 4,
    OP_TYPE_IO_IOCTL = 5,
    OP_TYPE_IO_SELECT = 6,
};

struct IoSyscallInfo {
    int32_t opType;
    int32_t fd;
    int64_t retValue;
    uint64_t directArg1;
    uint64_t directArg2;
    int64_t bufferLength;
};

struct IoOperationEvent {
    uint32_t sequence;
    uint32_t rfu;
    uint64_t timestamp;
    IoSyscallInfo info;
};
static_assert(sizeof(IoSyscallInfo) == 40);
static_assert(sizeof(IoOperationEvent) == sizeof(IoSyscallInfo) + 16);
static_assert(sizeof(OriginHookProcedure) == 56);

#endif //NCI_HOST_NATIVES_DAEMON_IPC_STRUCT_H
