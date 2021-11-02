// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-11-02.
//

#ifndef NCI_HOST_NATIVES_PTRACE_INJECT_ARCH_H
#define NCI_HOST_NATIVES_PTRACE_INJECT_ARCH_H

#include <cstdint>
#include <array>

int arch_ptrace_call_procedure_and_wait_aarch64(int pid, uintptr_t proc,
                                                uintptr_t *retval, const std::array<uintptr_t, 4> &args, int timeout);

int arch_ptrace_call_procedure_and_wait_arm(int pid, uintptr_t proc,
                                            uintptr_t *retval, const std::array<uintptr_t, 4> &args, int timeout);

int arch_ptrace_call_procedure_and_wait_amd64(int pid, uintptr_t proc,
                                              uintptr_t *retval, const std::array<uintptr_t, 4> &args, int timeout);

int arch_ptrace_call_procedure_and_wait_x86(int pid, uintptr_t proc,
                                            uintptr_t *retval, const std::array<uintptr_t, 4> &args, int timeout);

#endif //NCI_HOST_NATIVES_PTRACE_INJECT_ARCH_H
