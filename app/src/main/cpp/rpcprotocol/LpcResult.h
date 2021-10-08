//
// Created by kinit on 2021-10-02.
//

#ifndef RPCPROTOCOL_LPCRESULT_H
#define RPCPROTOCOL_LPCRESULT_H

#include "cstdint"
#include "string"

#include "ArgList.h"
#include "rpc_struct.h"
#include "RemoteException.h"

class LpcResult {
private:
    bool mIsValid = false;
    bool mHasException = false;
    uint32_t mErrorCode = 0;
    SharedBuffer mArgsBuffer;
public:
    [[nodiscard]] inline bool isValid() const noexcept {
        return mIsValid;
    }

    [[nodiscard]] inline bool isSuccessful() const noexcept {
        return mIsValid && (mErrorCode == 0);
    }

    [[nodiscard]] inline bool hasException() const noexcept {
        return mIsValid && mHasException;
    }

    inline void setError(LpcErrorCode errorCode) noexcept {
        mErrorCode = static_cast<uint32_t>(errorCode);
        mHasException = true;
        mIsValid = true;
    }

    [[nodiscard]] inline uint32_t getError() const noexcept {
        return mErrorCode;
    }

    inline void throwException(const RemoteException &exception) {
        mArgsBuffer = ArgList::Builder().pushArgs(exception.typeId, exception.statusCode, exception.message).build();
        mHasException = false;
        mErrorCode = 0;
        mIsValid = true;
    }

    template<class T>
    inline void setException(const RemoteException &exception) {
        throwException(exception);
    }

    inline void returnVoid() {
        mArgsBuffer = ArgList::Builder().build();
        mHasException = false;
        mErrorCode = 0;
        mIsValid = true;
    }

    template<class T>
    inline void setResult(const T &result) {
        returnResult(result);
    }

    template<class T>
    void returnResult(const T &result) {
        mArgsBuffer = ArgList::Builder().push(result).build();
        mHasException = false;
        mErrorCode = 0;
        mIsValid = true;
    }

    template<class T>
    [[nodiscard]] bool getResult(T *result) const {
        if (!mIsValid || mHasException || mArgsBuffer.get() == nullptr) {
            return false;
        }
        return ArgList(mArgsBuffer.get(), mArgsBuffer.size()).get<T>(result, 0);
    }

    [[nodiscard]] bool getException(RemoteException *exception) const {
        if (!mIsValid || !mHasException || mArgsBuffer.get() == nullptr) {
            return false;
        }
        uint32_t type = 0;
        uint32_t status = 0;
        std::string msg;
        if (ArgList args(mArgsBuffer.get(), mArgsBuffer.size());
                args.get(&type, 0) || args.get(&status, 1) || args.get(&msg, 2)) {
            *exception = {type, status, msg};
            return true;
        }
        return false;
    }

    inline void reset() noexcept {
        mIsValid = false;
        mHasException = false;
        mErrorCode = 0;
        mArgsBuffer.reset();
    }

    [[nodiscard]] SharedBuffer buildLpcResponsePacket(uint32_t sequence) const;

    [[nodiscard]] static LpcResult fromLpcRespPacketBuffer(const SharedBuffer &buffer);
};


#endif //RPCPROTOCOL_LPCRESULT_H
