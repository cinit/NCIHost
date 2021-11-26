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
using u32 = uint32_t;

struct user_regs_aarch64 {
    u64 regs[31];
    u64 sp;
    u64 pc;
    u64 pstate;
};
static_assert(sizeof(user_regs_aarch64) == 34 * 8, "user_regs_aarch64 size error");

struct user_regs_aarch32 {
    u32 uregs[18];
};
static_assert(sizeof(user_regs_aarch32) == 18 * 4, "user_regs_aarch32 size error");

#define ARM_cpsr      uregs[16]
#define ARM_pc        uregs[15]
#define ARM_lr        uregs[14]
#define ARM_sp        uregs[13]
#define ARM_ip        uregs[12]
#define ARM_fp        uregs[11]
#define ARM_r10       uregs[10]
#define ARM_r9        uregs[9]
#define ARM_r8        uregs[8]
#define ARM_r7        uregs[7]
#define ARM_r6        uregs[6]
#define ARM_r5        uregs[5]
#define ARM_r4        uregs[4]
#define ARM_r3        uregs[3]
#define ARM_r2        uregs[2]
#define ARM_r1        uregs[1]
#define ARM_r0        uregs[0]
#define ARM_ORIG_r0   uregs[17]

int arch_ptrace_call_procedure_and_wait_aarch64(int pid, uintptr_t proc, uintptr_t *retval,
                                                const std::array<uintptr_t, 4> &args, int timeout) {
    user_regs_aarch64 regs = {};
    if (ptrace_get_gp_regs_compat(pid, &regs, sizeof(user_regs_aarch64)) < 0) {
        return -errno;
    }
    user_regs_aarch64 tmp = regs;
    tmp.pc = proc;
    tmp.regs[0] = args[0];
    tmp.regs[1] = args[1];
    tmp.regs[2] = args[2];
    tmp.regs[3] = args[3];
    tmp.regs[30] = 0;// set LR = 0, trigger a SIGSEGV when procedure returns
    // TODO: handle ARMv8.3 pointer authentication
    if (ptrace_set_gp_regs_compat(pid, &tmp, sizeof(user_regs_aarch64)) < 0) {
        return -errno;
    }
    if (::ptrace(PTRACE_CONT, pid, 0, 0) < 0) {
        return -errno;
    }
    if (int status; (status = wait_for_signal(pid, timeout)) < 0) {
        return status;
    }
    if (ptrace_get_gp_regs_compat(pid, &tmp, sizeof(user_regs_aarch64)) < 0) {
        return -errno;
    }
    *retval = tmp.regs[0];
    if (ptrace_set_gp_regs_compat(pid, &regs, sizeof(user_regs_aarch64)) < 0) {
        return -errno;
    }
    return 0;
}

int arch_ptrace_call_procedure_and_wait_arm(int pid, uintptr_t proc,
                                            uintptr_t *retval, const std::array<uintptr_t, 4> &args, int timeout) {
    user_regs_aarch32 regs = {};
    if (ptrace_get_gp_regs_compat(pid, &regs, sizeof(user_regs_aarch32)) < 0) {
        return -errno;
    }
    user_regs_aarch32 tmp = regs;
    tmp.ARM_pc = uint32_t(proc);
    tmp.uregs[0] = uint32_t(args[0]);
    tmp.uregs[1] = uint32_t(args[1]);
    tmp.uregs[2] = uint32_t(args[2]);
    tmp.uregs[3] = uint32_t(args[3]);
    tmp.ARM_lr = 0;// set LR = 0, trigger a SIGSEGV when procedure returns
    if (ptrace_set_gp_regs_compat(pid, &tmp, sizeof(user_regs_aarch32)) < 0) {
        return -errno;
    }
    if (::ptrace(PTRACE_CONT, pid, 0, 0) < 0) {
        return -errno;
    }
    if (int status; (status = wait_for_signal(pid, timeout)) < 0) {
        return status;
    }
    if (ptrace_get_gp_regs_compat(pid, &tmp, sizeof(user_regs_aarch32)) < 0) {
        return -errno;
    }
    *retval = tmp.uregs[0];
    if (ptrace_set_gp_regs_compat(pid, &regs, sizeof(user_regs_aarch32)) < 0) {
        return -errno;
    }
    return 0;
}
