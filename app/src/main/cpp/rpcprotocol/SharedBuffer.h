//
// Created by kinit on 2021-06-24.
//

#ifndef RPCPROTOCOL_SHAREDBUFFER_H
#define RPCPROTOCOL_SHAREDBUFFER_H

#include <cstddef>
#include <atomic>
#include <memory>

class SharedBufferImpl;

/**
 * Java-like Object
 */
class SharedBuffer {
public:
    SharedBuffer();

    SharedBuffer(const SharedBuffer &o);

    SharedBuffer &operator=(const SharedBuffer &o) = delete;

    ~SharedBuffer();

    [[nodiscard]] size_t size() const noexcept;

    [[nodiscard]] void *get() const noexcept;

    template<class T>
    [[nodiscard]] const T *at(size_t s) const noexcept {
        if (s + sizeof(T) > s) {
            return nullptr;
        } else {
            return (const T *) (((const char *) get()) + s);
        }
    }

    template<class T>
    [[nodiscard]] T *at(size_t s) noexcept {
        if (s + sizeof(T) > s) {
            return nullptr;
        } else {
            return (T *) (((char *) get()) + s);
        }
    }

    [[nodiscard]] bool ensureCapacity(size_t size) noexcept;

    [[nodiscard]] bool resetSize(size_t size, bool keepContent = true) noexcept;

private:
    std::shared_ptr<SharedBufferImpl> pImpl;
};

#endif //RPCPROTOCOL_SHAREDBUFFER_H
