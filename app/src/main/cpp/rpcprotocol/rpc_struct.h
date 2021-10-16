#ifndef RPC_STRUCT_H
#define RPC_STRUCT_H

#include <cstdint>

namespace ipcprotocol {

typedef struct TrxnPacketHeader {
    uint32_t length; // size of entire packet
    uint32_t type;
} TrxnPacketHeader;

typedef enum TrxnType : uint32_t {
    TRXN_TYPE_RPC = 1,
    TRXN_TYPE_EVENT = 2
} TrxnType;

enum class LpcErrorCode : uint32_t {
    ERR_SUCCESS = 0,
    ERR_BROKEN_CONN = 0x01,
    ERR_LOCAL_INTERNAL_ERROR = 0x3,
    ERR_BAD_BUFFER = 0x12,
    ERR_INVALID_FUNCTION = 0x13,
    ERR_NO_LPC_HANDLER = 0x14,
    ERR_BAD_ARGUMENT = 0x15,
    ERR_REMOTE_INTERNAL_ERROR = 0x21,
};

typedef struct LpcTransactionHeader {
    TrxnPacketHeader header;
    uint32_t sequence;
    uint32_t funcId;
    uint32_t rpcFlags;
    uint32_t errorCode;
} LpcTransactionHeader;

constexpr uint32_t LPC_FLAG_RESULT_EXCEPTION = 0x1;
constexpr uint32_t LPC_FLAG_ERROR = 0x2;

typedef struct EventTransactionHeader {
    TrxnPacketHeader header;
    uint32_t sequence;
    uint32_t eventId;
    uint32_t eventFlags;
    uint32_t reserved;
} EventTransactionHeader;

static_assert(sizeof(TrxnPacketHeader) == 8, "TrxnPacketHeader size error");
static_assert(sizeof(LpcTransactionHeader) == 24, "RpcTransactionHeader size error");
static_assert(sizeof(EventTransactionHeader) == 24, "EventTransactionHeader size error");

}

#endif //RPC_STRUCT_H
