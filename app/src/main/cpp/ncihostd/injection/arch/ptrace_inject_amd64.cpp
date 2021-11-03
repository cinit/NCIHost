// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-10-31.
//
#include <sys/ptrace.h>
#include <cstdint>
#include <cerrno>

#include "ptrace_inject_arch.h"
#include "ptrace_inject_utils.h"

using namespace inject;
using u64 = uint64_t;

struct amd64_user_regs_struct {
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 rbp;
    u64 rbx;
    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;
    u64 rax;
    u64 rcx;
    u64 rdx;
    u64 rsi;
    u64 rdi;
    u64 orig_rax;
    u64 rip;
    u64 cs;
    u64 eflags;
    u64 rsp;
    u64 ss;
    u64 fs_base;
    u64 gs_base;
    u64 ds;
    u64 es;
    u64 fs;
    u64 gs;
};

int arch_ptrace_call_procedure_and_wait_amd64(int pid, uintptr_t proc,
                                              uintptr_t *retval, const std::array<uintptr_t, 4> &args, int timeout) {
    amd64_user_regs_struct regs = {};
    if (::ptrace(PTRACE_GETREGS, pid, 0, &regs) < 0) {
        return -errno;
    }
    amd64_user_regs_struct tmp = regs;
    tmp.rip = proc;
    tmp.rdi = args[0];
    tmp.rsi = args[1];
    tmp.rdx = args[2];
    tmp.rcx = args[3];
    // align rsp to 16
    while ((tmp.rsp & 0xf) != 0) {
        tmp.rsp--;
    }
    // TODO: handle Intel shadow stack CET
    // push 0 as return address, trigger a SIGSEGV when procedure returns
    const uintptr_t tmp_null = 0;
    tmp.rsp -= 8;
    ptrace_write_data(pid, tmp.rsp, &tmp_null, 8);
    if (::ptrace(PTRACE_SETREGS, pid, 0, &tmp) < 0) {
        return -errno;
    }
    if (::ptrace(PTRACE_CONT, pid, 0, 0) < 0) {
        return -errno;
    }
    int stopSignal;
    if ((stopSignal = wait_for_signal(pid, timeout)) < 0) {
        return stopSignal;
    }
    if (::ptrace(PTRACE_GETREGS, pid, 0, &tmp) < 0) {
        return -errno;
    }
    *retval = tmp.rax;
    if (::ptrace(PTRACE_SETREGS, pid, 0, &regs) < 0) {
        return -errno;
    }
    return 0;
}
