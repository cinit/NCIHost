//
// Created by kinit on 2021-10-02.
//

#ifndef RPCPROTOCOL_LPCRESULT_H
#define RPCPROTOCOL_LPCRESULT_H

#include <cstdint>
#include <string>

#include "ArgList.h"
#include "rpc_struct.h"
#include "RemoteException.h"

template<class T>
class TypedLpcResult;

class LpcResult {
private:
    bool mIsValid = false;
    bool mHasException = false;
    uint32_t mErrorCode = 0;
    SharedBuffer mArgsBuffer;
public:
    LpcResult();

    LpcResult(const LpcResult &result);

    LpcResult &operator=(const LpcResult &result);

    ~LpcResult();

    template<class T>
    explicit LpcResult(const TypedLpcResult<T> &result) :
            mIsValid(result.mResult.mIsValid),
            mHasException(result.mResult.mHasException),
            mErrorCode(result.mResult.mErrorCode),
            mArgsBuffer(result.mResult.mArgsBuffer) {}

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

template<class T>
class TypedLpcResult {
private:
    LpcResult mResult;
    friend LpcResult;
public:
    TypedLpcResult() = default;

    inline TypedLpcResult(const T &value) {
        setResult(value);
    }

    TypedLpcResult(const TypedLpcResult &result) = default;

    TypedLpcResult &operator=(const TypedLpcResult &result) = default;

    ~TypedLpcResult() = default;

    explicit TypedLpcResult(const LpcResult &result) : mResult(result) {
    }

    [[nodiscard]] inline bool isValid() const noexcept {
        return mResult.isValid();
    }

    [[nodiscard]] inline bool isSuccessful() const noexcept {
        return mResult.isSuccessful();
    }

    [[nodiscard]] inline bool hasException() const noexcept {
        return mResult.hasException();
    }

    inline void setError(LpcErrorCode errorCode) noexcept {
        mResult.setError(errorCode);
    }

    [[nodiscard]] inline uint32_t getError() const noexcept {
        return mResult.getError();
    }

    inline void throwException(const RemoteException &exception) {
        mResult.throwException(exception);
    }

    inline void setException(const RemoteException &exception) {
        mResult.throwException(exception);
    }

    inline void returnVoid() {
        mResult.returnVoid();
    }

    inline void setResult(const T &result) {
        mResult.setResult<T>(result);
    }

    inline void returnResult(const T &result) {
        mResult.returnResult<T>(result);
    }

    [[nodiscard]] inline bool getResult(T *result) const {
        return mResult.getResult<T>(result);
    }

    [[nodiscard]] inline bool getException(RemoteException *exception) const {
        return mResult.getException(exception);
    }

    inline void reset() noexcept {
        mResult.reset();
    }
};

template<>
class TypedLpcResult<void> {
private:
    LpcResult mResult;
    friend LpcResult;
public:
    TypedLpcResult() = default;

    TypedLpcResult(const TypedLpcResult &result) = default;

    TypedLpcResult &operator=(const TypedLpcResult &result) = default;

    ~TypedLpcResult() = default;

    explicit TypedLpcResult(const LpcResult &result) : mResult(result) {
    }

    [[nodiscard]] inline bool isValid() const noexcept {
        return mResult.isValid();
    }

    [[nodiscard]] inline bool isSuccessful() const noexcept {
        return mResult.isSuccessful();
    }

    [[nodiscard]] inline bool hasException() const noexcept {
        return mResult.hasException();
    }

    inline void setError(LpcErrorCode errorCode) noexcept {
        mResult.setError(errorCode);
    }

    [[nodiscard]] inline uint32_t getError() const noexcept {
        return mResult.getError();
    }

    inline void throwException(const RemoteException &exception) {
        mResult.throwException(exception);
    }

    inline void setException(const RemoteException &exception) {
        mResult.throwException(exception);
    }

    inline void returnVoid() {
        mResult.returnVoid();
    }

    [[nodiscard]] inline bool getException(RemoteException *exception) const {
        return mResult.getException(exception);
    }

    inline void reset() noexcept {
        mResult.reset();
    }
};

#endif //RPCPROTOCOL_LPCRESULT_H
