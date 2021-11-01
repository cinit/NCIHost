// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-11-01.
//

#ifndef NCI_HOST_NATIVES_PTRACE_INJECT_COMMON_H
#define NCI_HOST_NATIVES_PTRACE_INJECT_COMMON_H

#include <cstdint>

namespace inject {

int ptrace_call_procedure_and_wait(int arch, int pid, int timeout, uintptr_t *retval,
                                   uintptr_t r0, uintptr_t r1, uintptr_t r2, uintptr_t r3);

}

#endif //NCI_HOST_NATIVES_PTRACE_INJECT_COMMON_H
