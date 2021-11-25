//
// Created by kinit on 2021-10-02.
//

#include "LpcResult.h"
#include "rpc_struct.h"

using namespace ipcprotocol;

LpcResult::LpcResult(const LpcResult &result) = default;

LpcResult::LpcResult() = default;

LpcResult &LpcResult::operator=(const LpcResult &result) = default;

LpcResult::~LpcResult() = default;

LpcResult LpcResult::fromLpcRespPacketBuffer(const SharedBuffer &buffer) {
    LpcResult result;
    constexpr size_t headerSize = sizeof(LpcTransactionHeader);
    if (buffer.get() == nullptr || buffer.size() < headerSize) {
        return result;
    }
    if (buffer.size() > headerSize) {
        result.mArgsBuffer.ensureCapacity(buffer.size() - headerSize);
        memcpy(result.mArgsBuffer.get(), buffer.at<char>(headerSize), buffer.size() - headerSize);
    }
    result.mErrorCode = buffer.at<LpcTransactionHeader>(0)->errorCode;
    result.mHasException = (buffer.at<LpcTransactionHeader>(0)->rpcFlags
                            & LPC_FLAG_RESULT_EXCEPTION) != 0;
    result.mIsValid = result.mErrorCode != 0
                      || ArgList(buffer.at<char>(headerSize), buffer.size()).isValid();
    return result;
}

SharedBuffer LpcResult::buildLpcResponsePacket(uint32_t sequence, uint32_t proxyId) const {
    SharedBuffer result;
    if (mIsValid) {
        result.ensureCapacity(sizeof(LpcTransactionHeader) + mArgsBuffer.size());
        memcpy(result.at<char>(sizeof(LpcTransactionHeader)), mArgsBuffer.get(), mArgsBuffer.size());
        auto *h = result.at<LpcTransactionHeader>(0);
        h->header.length = (uint32_t) result.size();
        h->header.type = TrxnType::TRXN_TYPE_RPC;
        h->header.proxyId = proxyId;
        h->header.rfu4_0 = 0;
        h->funcId = 0;
        h->sequence = sequence;
        h->rpcFlags = (mHasException ? LPC_FLAG_RESULT_EXCEPTION : 0) | (mErrorCode == 0 ? 0 : LPC_FLAG_ERROR);
        h->errorCode = mErrorCode;
    }
    return result;
}
