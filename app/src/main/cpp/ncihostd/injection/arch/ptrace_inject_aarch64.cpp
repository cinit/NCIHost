// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-10-31.
//
#include <sys/ptrace.h>
#include <cerrno>
#include <cstdint>

#include "ptrace_inject_arch.h"
#include "ptrace_inject_utils.h"

using namespace inject;
using u64 = uint64_t;

/*
 * arch/arm64/include/uapi/asm/ptrace.h
 * User structures for general purpose, floating point and debug registers.
 */
struct aarch64_user_pt_regs {
    u64 regs[31];
    u64 sp;
    u64 pc;
    u64 pstate;
};

int arch_ptrace_call_procedure_and_wait_aarch64(int pid, uintptr_t proc,
                                                uintptr_t *retval, const std::array<uintptr_t, 4> &args, int timeout) {
    aarch64_user_pt_regs regs = {};
    if (::ptrace(PTRACE_GETREGS, pid, 0, &regs) < 0) {
        return -errno;
    }
    aarch64_user_pt_regs tmp = regs;
    tmp.pc = proc;
    tmp.regs[0] = args[0];
    tmp.regs[1] = args[1];
    tmp.regs[2] = args[2];
    tmp.regs[3] = args[3];
    tmp.regs[30] = 0;// set LR = 0, trigger a SIGSEGV when procedure returns
    if (::ptrace(PTRACE_SETREGS, pid, 0, &tmp) < 0) {
        return -errno;
    }
    if (::ptrace(PTRACE_CONT, pid, 0, 0) < 0) {
        return -errno;
    }
    if (int status; (status = wait_for_signal(pid, timeout)) < 0) {
        return status;
    }
    if (::ptrace(PTRACE_GETREGS, pid, 0, &tmp) < 0) {
        return -errno;
    }
    *retval = tmp.regs[0];
    if (::ptrace(PTRACE_SETREGS, pid, 0, &regs) < 0) {
        return -errno;
    }
    return 0;
}
