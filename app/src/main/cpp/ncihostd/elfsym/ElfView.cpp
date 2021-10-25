//
// Created by kinit on 2021-10-25.
//

#include "ElfView.h"

using namespace elfsym;

int ElfView::getPointerSize() const noexcept {
    if (!isValid()) {
        return 0;
    }
    char type = *mRva.at<char>(4);
    if (type == 1) {
        return 4;
    } else if (type == 2) {
        return 8;
    } else {
        return 0;
    }
}

uint64_t ElfView::getExtSymGotRva(const char *symbol) const {
    if (symbol == nullptr) {
        return 0;
    }
    int pointerSize = getPointerSize();
    if (pointerSize == 4) {
        // ELF32
    } else if (pointerSize == 8) {
        // ELF64
    } else {
        return 0;
    }
}
