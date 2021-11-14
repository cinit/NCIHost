// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-11-01.
//

#ifndef NCI_HOST_NATIVES_PTRACE_INJECT_UTILS_H
#define NCI_HOST_NATIVES_PTRACE_INJECT_UTILS_H

#include <cstdint>
#include <array>

namespace inject {

[[nodiscard]]
int ptrace_call_procedure(int arch, int pid, uintptr_t proc,
                          uintptr_t *retval, const std::array<uintptr_t, 4> &args, int timeout = 100);

[[nodiscard]]
int ptrace_read_data(int pid, uintptr_t addr, void *buffer, int size);

[[nodiscard]]
int ptrace_write_data(int pid, uintptr_t addr, const void *buffer, int size);

[[nodiscard]]
int wait_for_signal(int tid, int timeout);

}

#endif //NCI_HOST_NATIVES_PTRACE_INJECT_UTILS_H
