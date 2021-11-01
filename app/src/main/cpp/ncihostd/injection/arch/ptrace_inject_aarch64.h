// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-10-31.
//

#ifndef NCI_HOST_NATIVES_PTRACE_INJECT_AARCH64_H
#define NCI_HOST_NATIVES_PTRACE_INJECT_AARCH64_H

#include <cstdint>

int arch_ptrace_call_procedure_and_wait_aarch64(int pid, int timeout, uintptr_t *retval,
                                                uintptr_t r0, uintptr_t r1, uintptr_t r2, uintptr_t r3);

#endif //NCI_HOST_NATIVES_PTRACE_INJECT_AARCH64_H
