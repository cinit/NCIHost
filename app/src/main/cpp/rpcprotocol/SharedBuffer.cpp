//
// Created by kinit on 2021-06-24.
//

#include "SharedBuffer.h"

#include <mutex>
#include <algorithm>
#include <cstring>
#include <malloc.h>

class SharedBufferImpl {
private:
    mutable std::mutex mMutex;
    void *mBuffer = nullptr;
    size_t mSize = 0;
public:

    SharedBufferImpl() = default;

    ~SharedBufferImpl() noexcept {
        std::scoped_lock _(mMutex);
        if (mBuffer != nullptr) {
            free(mBuffer);
            mBuffer = nullptr;
            mSize = 0;
        }
    }

    [[nodiscard]] void *get() const noexcept {
        std::scoped_lock _(mMutex);
        return mBuffer;
    }

    [[nodiscard]] size_t size() const noexcept {
        std::scoped_lock _(mMutex);
        return mSize;
    }

    [[nodiscard]] bool resetSize(size_t size, bool keepContent) noexcept {
        std::scoped_lock _(mMutex);
        if (mBuffer != nullptr) {
            void *newBuffer = malloc(size);
            if (newBuffer == nullptr) {
                return false;
            }
            if (keepContent) {
                size_t cpSize = std::min(mSize, size);
                memcpy(newBuffer, mBuffer, cpSize);
            }
            free(mBuffer);
            mBuffer = newBuffer;
            mSize = size;
        } else {
            mBuffer = malloc(size);
            if (mBuffer == nullptr) {
                return false;
            }
            mSize = size;
        }
        return true;
    }
};

SharedBuffer::SharedBuffer() = default;

SharedBuffer::SharedBuffer(const SharedBuffer &o) = default;

SharedBuffer::~SharedBuffer() = default;

size_t SharedBuffer::size() const {
    const SharedBufferImpl *p = pImpl.get();
    if (p == nullptr) {
        return 0;
    } else {
        return p->size();
    }
}

void *SharedBuffer::get() const {
    const SharedBufferImpl *p = pImpl.get();
    if (p == nullptr) {
        return nullptr;
    } else {
        return p->get();
    }
}

bool SharedBuffer::ensureCapacity(size_t size) {
    SharedBufferImpl *p = pImpl.get();
    if (p == nullptr) {
        p = new SharedBufferImpl();
        pImpl.reset(p);
        return p->resetSize(size, false);
    } else {
        if (p->size() < size) {
            return p->resetSize(size, true);
        } else {
            return true;
        }
    }
}

bool SharedBuffer::resetSize(size_t size, bool keepContent) {
    SharedBufferImpl *p = pImpl.get();
    if (p == nullptr) {
        p = new SharedBufferImpl();
        pImpl.reset(p);
        return p->resetSize(size, keepContent);
    } else {
        return p->resetSize(size, keepContent);
    }
}
