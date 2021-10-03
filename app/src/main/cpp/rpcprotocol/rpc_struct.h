#include <cstdint>

typedef struct TrxnPacketHeader {
    uint32_t length; // for payload
    uint32_t type;
} TrxnPacketHeader;

typedef enum TrxnType : uint32_t {
    TRXN_TYPE_RPC = 1,
    TRXN_TYPE_EVENT = 2
} TrxnType;

typedef struct RpcTransactionHeader {
    uint32_t sequence;
    uint32_t funcId;
    uint32_t rpcFlags;
    uint32_t reserved;
} RpcTransactionHeader;

constexpr uint32_t RPC_FLAG_EXCEPTION = 0x1;

typedef struct EventTransactionHeader {
    uint32_t sequence;
    uint32_t eventId;
    uint32_t eventFlags;
    uint32_t reserved;
} EventTransactionHeader;

static_assert(sizeof(TrxnPacketHeader) == 8, "TrxnPacketHeader size error");
static_assert(sizeof(RpcTransactionHeader) == 16, "RpcTransactionHeader size error");
static_assert(sizeof(EventTransactionHeader) == 16, "EventTransactionHeader size error");
