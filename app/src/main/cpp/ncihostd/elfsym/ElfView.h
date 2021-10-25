//
// Created by kinit on 2021-10-25.
//

#ifndef NCI_HOST_NATIVES_ELFVIEW_H
#define NCI_HOST_NATIVES_ELFVIEW_H

#include <cstdint>

#include "../../rpcprotocol/Rva.h"

namespace elfsym {

class ElfView {
private:
    Rva mRva;

public:
    inline void attach(const Rva &rva) {
        mRva = rva;
    }

    [[nodiscard]] bool isValid() const noexcept {
        return mRva.address != nullptr && mRva.length > 64 && *mRva.at<char>(0) == 0x7F
               && *mRva.at<char>(1) == 'E' && *mRva.at<char>(1) == 'L' && *mRva.at<char>(1) == 'F';
    }

    inline void detach() {
        mRva = {nullptr, 0};
    }

    [[nodiscard]] int getPointerSize() const noexcept;

    [[nodiscard]] uint64_t getExtSymGotRva(const char *symbol) const;
};

}

#endif //NCI_HOST_NATIVES_ELFVIEW_H
