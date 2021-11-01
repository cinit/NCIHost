// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-11-01.
//
#include <cerrno>
#include "ptrace_inject_aarch64.h"

#include "ptrace_inject_common.h"

namespace inject {

constexpr int ARCH_X86 = 3;
constexpr int ARCH_X86_64 = 62;
constexpr int ARCH_ARM = 40;
constexpr int ARCH_AARCH64 = 183;

int ptrace_call_procedure_and_wait(int arch, int pid, int timeout, uintptr_t *retval,
                                   uintptr_t r0, uintptr_t r1, uintptr_t r2, uintptr_t r3) {
    switch (arch) {
        case ARCH_X86:
        case ARCH_X86_64:
        case ARCH_ARM: {
            return -ENOSYS;
        }
        case ARCH_AARCH64: {
            return arch_ptrace_call_procedure_and_wait_aarch64(pid, timeout, retval, r0, r1, r2, r3);
        }
        default: {
            return -ENOSYS;
        }
    }
}

}
