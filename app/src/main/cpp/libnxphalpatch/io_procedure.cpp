//
// Created by kinit on 2021-10-17.
//
#include <cstdint>

#include "io_procedure.h"

using InjectInfo = struct {
    char daemonVersion[32];
    char socketName[64];
};

extern "C" void NxpHalPatchInit(void *p) {
    const auto *info = static_cast<const InjectInfo *>(p);
    if (info == nullptr) {
        return;
    }


}
