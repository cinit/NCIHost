#ifndef RPC_STRUCT_H
#define RPC_STRUCT_H

#include <cstdint>

namespace ipcprotocol {

enum class TrxnType : uint32_t {
    TRXN_TYPE_RPC = 1,
    TRXN_TYPE_EVENT = 2
};

struct TrxnPacketHeader {
    uint32_t length; // size of entire packet
    TrxnType type;
    uint32_t proxyId; // 0 - 255 is reserved
    uint32_t rfu4_0;
};

enum class LpcErrorCode : uint32_t {
    ERR_SUCCESS = 0,
    ERR_BROKEN_CONN = 0x01,
    ERR_LOCAL_INTERNAL_ERROR = 0x3,
    ERR_NO_REMOTE_OBJECT = 0x11,
    ERR_BAD_REQUEST = 0x12,
    ERR_UNKNOWN_REQUEST = 0x13,
    ERR_NO_LPC_HANDLER = 0x14,
    ERR_INVALID_ARGUMENT = 0x15,
    ERR_REMOTE_INTERNAL_ERROR = 0x21,
    ERR_TIMEOUT_IN_CRITICAL_CONTEXT = 0x40,
};

struct LpcTransactionHeader {
    TrxnPacketHeader header;
    uint32_t sequence;
    uint32_t funcId;
    uint32_t rpcFlags;
    uint32_t errorCode;
};

constexpr uint32_t LPC_FLAG_RESULT_EXCEPTION = 0x1;
constexpr uint32_t LPC_FLAG_ERROR = 0x2;

struct EventTransactionHeader {
    TrxnPacketHeader header;
    uint32_t sequence;
    uint32_t eventId;
    uint32_t eventFlags;
    uint32_t reserved;
};

static_assert(sizeof(TrxnPacketHeader) == 16, "TrxnPacketHeader size error");
static_assert(sizeof(LpcTransactionHeader) == sizeof(TrxnPacketHeader) + 16, "RpcTransactionHeader size error");
static_assert(sizeof(EventTransactionHeader) == sizeof(TrxnPacketHeader) + 16, "EventTransactionHeader size error");

}

#endif //RPC_STRUCT_H
