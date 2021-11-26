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
using u32 = uint32_t;
using u16 = unsigned short;

struct user_regs_struct_i386 {
    u32 ebx, ecx, edx, esi, edi, ebp, eax;
    u16 ds, __ds, es, __es;
    u16 fs, __fs, gs, __gs;
    u32 orig_eax, eip;
    u16 cs, __cs;
    u32 eflags, esp;
    u16 ss, __ss;
};
static_assert(sizeof(user_regs_struct_i386) == 17 * 4, "user_regs_struct_i386 size error");

struct user_regs_struct_amd64 {
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
static_assert(sizeof(user_regs_struct_amd64) == 27 * 8, "user_regs_struct_amd64 size error");

int arch_ptrace_call_procedure_and_wait_amd64(int pid, uintptr_t proc, uintptr_t *retval,
                                              const std::array<uintptr_t, 4> &args, int timeout) {
    user_regs_struct_amd64 regs = {};
    if (ptrace_get_gp_regs_compat(pid, &regs, sizeof(user_regs_struct_amd64)) < 0) {
        return -errno;
    }
    user_regs_struct_amd64 tmp = regs;
    tmp.rip = proc;
    tmp.rax = 0; // put syscall restart aside
    tmp.rdi = args[0];
    tmp.rsi = args[1];
    tmp.rdx = args[2];
    tmp.rcx = args[3];
    tmp.rsp -= 64;
    // align rsp to 16
    while (((tmp.rsp - 16) & 0xf) != 0) {
        tmp.rsp--;
    }
    // TODO: handle Intel shadow stack CET
    // push 0 as return address, trigger a SIGSEGV when procedure returns
    const uintptr_t tmp_null = 0;
    tmp.rsp -= 8;
    if (int err; (err = ptrace_write_data(pid, tmp.rsp, &tmp_null, 8)) != 0) {
        return err;
    }
    if (ptrace_set_gp_regs_compat(pid, &tmp, sizeof(user_regs_struct_amd64)) < 0) {
        return -errno;
    }
    if (::ptrace(PTRACE_CONT, pid, 0, 0) < 0) {
        return -errno;
    }
    int stopSignal;
    if ((stopSignal = wait_for_signal(pid, timeout)) < 0) {
        return stopSignal;
    }
    if (ptrace_get_gp_regs_compat(pid, &tmp, sizeof(user_regs_struct_amd64)) < 0) {
        return -errno;
    }
    if (ptrace_set_gp_regs_compat(pid, &regs, sizeof(user_regs_struct_amd64)) < 0) {
        return -errno;
    }
    if (tmp.rip == 0) {
        *retval = tmp.rax;
        return 0;
    } else {
        return -EFAULT;
    }
}

int arch_ptrace_call_procedure_and_wait_x86(int pid, uintptr_t proc, uintptr_t *retval,
                                            const std::array<uintptr_t, 4> &args, int timeout) {
    user_regs_struct_i386 regs = {};
    if (ptrace_get_gp_regs_compat(pid, &regs, sizeof(user_regs_struct_i386)) < 0) {
        return -errno;
    }
    user_regs_struct_i386 tmp = regs;
    tmp.eip = uint32_t(proc);
    const uint32_t argStack[5] = {0, uint32_t(args[0]), uint32_t(args[1]), uint32_t(args[2]), uint32_t(args[3])};
    static_assert(sizeof(argStack) == 20);
    tmp.eax = 0; // put syscall restart aside
    tmp.esp -= 64;
    // align rsp to 8
    while (((tmp.esp - 8) & 0x7) != 0) {
        tmp.esp--;
    }
    // TODO: handle Intel shadow stack CET
    tmp.esp -= 4;
    // push args to stack, 0 as return address, trigger a SIGSEGV when procedure returns
    if (int err; (err = ptrace_write_data(pid, tmp.esp, &argStack, 20)) != 0) {
        return err;
    }
    if (ptrace_set_gp_regs_compat(pid, &tmp, sizeof(user_regs_struct_i386)) < 0) {
        return -errno;
    }
    if (::ptrace(PTRACE_CONT, pid, 0, 0) < 0) {
        return -errno;
    }
    int stopSignal;
    if ((stopSignal = wait_for_signal(pid, timeout)) < 0) {
        return stopSignal;
    }
    if (ptrace_get_gp_regs_compat(pid, &tmp, sizeof(user_regs_struct_i386)) < 0) {
        return -errno;
    }
    if (ptrace_set_gp_regs_compat(pid, &regs, sizeof(user_regs_struct_i386)) < 0) {
        return -errno;
    }
    if (tmp.eip == 0) {
        *retval = tmp.eax;
        return 0;
    } else {
        return -EFAULT;
    }
}
