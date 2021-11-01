// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-10-28.
//

#ifndef NCI_HOST_NATIVES_FILEMEMMAP_H
#define NCI_HOST_NATIVES_FILEMEMMAP_H

#include <cstddef>

class FileMemMap {
private:
    void *mAddress = nullptr;
    size_t mMapLength = 0;
    size_t mLength = 0;

public:
    FileMemMap() = default;

    ~FileMemMap() noexcept;

    FileMemMap(const FileMemMap &) = delete;

    FileMemMap &operator=(const FileMemMap &other) = delete;

    [[nodiscard]] int mapFilePath(const char *path, bool readOnly = true, size_t length = 0);

    [[nodiscard]] int mapFileDescriptor(int fd, bool readOnly = true, size_t length = 0, bool shared = false);

    [[nodiscard]] inline void *getAddress() const noexcept {
        return mAddress;
    }

    [[nodiscard]] inline size_t getLength() const noexcept {
        return mLength;
    }

    [[nodiscard]] inline bool isValid() const noexcept {
        return mAddress != nullptr && mLength != 0;
    }

    void unmap() noexcept;

    void detach() noexcept;
};

#endif //NCI_HOST_NATIVES_FILEMEMMAP_H
