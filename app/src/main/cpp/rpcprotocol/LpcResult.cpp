//
// Created by kinit on 2021-10-02.
//

#include "LpcResult.h"
#include "rpc_struct.h"

LpcResult LpcResult::fromLpcRespPacketBuffer(const SharedBuffer &buffer) {
    LpcResult result;
    if (buffer.get() == nullptr || buffer.size() < 24) {
        return result;
    }
    if (buffer.size() > 24) {
        result.mBuffer.ensureCapacity(buffer.size() - 24);
        memcpy(result.mBuffer.get(), buffer.get(), buffer.size() - 24);
    }
    result.mErrorCode = buffer.at<LpcTransactionHeader>(0)->errorCode;
    result.mHasException = (buffer.at<LpcTransactionHeader>(0)->rpcFlags
                            & LPC_FLAG_RESULT_EXCEPTION) != 0;
    result.mIsValid = result.mErrorCode != 0
                      || ArgList(buffer.at<char>(sizeof(LpcTransactionHeader)), buffer.size()).isValid();
    return result;
}

SharedBuffer LpcResult::buildLpcResponsePacket(uint32_t sequence) const {
    SharedBuffer result;
    if (mIsValid) {
        result.ensureCapacity(sizeof(LpcTransactionHeader) + mBuffer.size());
        memcpy(result.at<char>(sizeof(LpcTransactionHeader)), mBuffer.get(), mBuffer.size());
        auto *h = result.at<LpcTransactionHeader>(0);
        h->header.length = (uint32_t) result.size();
        h->header.type = TrxnType::TRXN_TYPE_RPC;
        h->funcId = 0;
        h->sequence = sequence;
        h->rpcFlags = (mHasException ? LPC_FLAG_RESULT_EXCEPTION : 0) | (mErrorCode == 0 ? 0 : LPC_FLAG_ERROR);
        h->errorCode = mErrorCode;
    }
    return result;
}
