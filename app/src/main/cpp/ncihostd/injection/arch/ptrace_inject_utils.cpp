// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-11-01.
//
#include <cerrno>
#include <array>
#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include "ptrace_inject_arch.h"

#include "ptrace_inject_utils.h"

namespace inject {

constexpr int ARCH_X86 = 3;
constexpr int ARCH_X86_64 = 62;
constexpr int ARCH_ARM = 40;
constexpr int ARCH_AARCH64 = 183;

constexpr int NT_PRSTATUS = 1;

int ptrace_call_procedure(int arch, int pid, uintptr_t proc,
                          uintptr_t *retval, const std::array<uintptr_t, 4> &args, int timeout) {
    switch (arch) {
        case ARCH_X86:
            return arch_ptrace_call_procedure_and_wait_x86(pid, proc, retval, args, timeout);
        case ARCH_X86_64:
            return arch_ptrace_call_procedure_and_wait_amd64(pid, proc, retval, args, timeout);
        case ARCH_ARM:
            return arch_ptrace_call_procedure_and_wait_arm(pid, proc, retval, args, timeout);
        case ARCH_AARCH64:
            return arch_ptrace_call_procedure_and_wait_aarch64(pid, proc, retval, args, timeout);
        default:
            return -ENOSYS;
    }
}

int ptrace_read_data(int pid, uintptr_t addr, void *buffer, int size) {
    int offset = 0;
    if ((addr / sizeof(void *) * sizeof(void *)) != addr) {
        uintptr_t tmp = 0;
        errno = 0;
        if ((tmp = ::ptrace(PTRACE_PEEKDATA, pid, addr / sizeof(void *) * sizeof(void *), 0)) == -1
            && errno != 0) {
            return -errno;
        }
        auto count = addr - addr / sizeof(void *) * sizeof(void *);
        offset = int(count);
        memcpy(buffer, ((char *) &tmp) + sizeof(void *) - count, count);
    }
    uintptr_t alignedStart = (addr + sizeof(void *) - 1) / sizeof(void *) * sizeof(void *);
    auto alignedWordCount = (size - offset) / sizeof(void *);
    for (int i = 0; i < alignedWordCount; i++) {
        uintptr_t tmp;
        errno = 0;
        if ((tmp = ::ptrace(PTRACE_PEEKDATA, pid, alignedStart + i * sizeof(void *), 0)) == -1 && errno != 0) {
            return -errno;
        }
        memcpy((char *) buffer + offset, &tmp, sizeof(void *));
        offset += sizeof(void *);
    }
    if (int tailSize = size - offset; tailSize > 0) {
        uintptr_t rptr = alignedStart + alignedWordCount * sizeof(void *);
        uintptr_t tmp = 0;
        errno = 0;
        if ((tmp = ::ptrace(PTRACE_PEEKDATA, pid, rptr, 0)) == -1 && errno != 0) {
            return -errno;
        }
        memcpy((char *) buffer + offset, &tmp, tailSize);
    }
    return 0;
}

int ptrace_write_data(int pid, uintptr_t addr, const void *buffer, int size) {
    int offset = 0;
    if ((addr / sizeof(void *) * sizeof(void *)) != addr) {
        uintptr_t tmp = 0;
        errno = 0;
        if ((tmp = ::ptrace(PTRACE_PEEKDATA, pid, addr / sizeof(void *) * sizeof(void *), 0)) == -1
            && errno != 0) {
            return -errno;
        }
        auto count = addr - addr / sizeof(void *) * sizeof(void *);
        offset = int(count);
        memcpy(((char *) &tmp) + sizeof(void *) - count, buffer, count);
        if (::ptrace(PTRACE_POKEDATA, pid, addr / sizeof(void *) * sizeof(void *), tmp) == -1) {
            return -errno;
        }
    }
    uintptr_t alignedStart = (addr + sizeof(void *) - 1) / sizeof(void *) * sizeof(void *);
    auto alignedWordCount = (size - offset) / sizeof(void *);
    for (int i = 0; i < alignedWordCount; i++) {
        if (::ptrace(PTRACE_POKEDATA, pid, alignedStart + i * sizeof(void *),
                     *(const uintptr_t *) (((const char *) buffer) + offset)) == -1) {
            return -errno;
        }
        offset += sizeof(void *);
    }
    if (int tailSize = size - offset; tailSize > 0) {
        uintptr_t rptr = alignedStart + alignedWordCount * sizeof(void *);
        uintptr_t tmp = 0;
        errno = 0;
        if ((tmp = ::ptrace(PTRACE_PEEKDATA, pid, rptr, 0)) == -1 && errno != 0) {
            return -errno;
        }
        memcpy(&tmp, ((const char *) buffer) + offset, tailSize);
        if (::ptrace(PTRACE_POKEDATA, pid, rptr, tmp) == -1) {
            return -errno;
        }
    }
    return 0;
}

int wait_for_signal(pid_t tid, int timeout) {
    int timeleft = timeout;
    while (true) {
        int status;
        pid_t n = waitpid(tid, &status, WNOHANG);
        if (n == -1) {
            return -errno;
        } else if (n == tid) {
            if (WIFSTOPPED(status)) {
                return WSTOPSIG(status);
            } else {
                // This is the only circumstance under which we can allow a detach
                // to fail with ESRCH, which indicates the tid has exited.
                return -1;
            }
        }
        if (timeleft <= 0) {
            return -ETIMEDOUT;
        }
        usleep(1000);
        timeleft--;
    }
}

int ptrace_get_gp_regs_compat(int pid, void *user_regs, size_t size) {
    if (iovec iov = {user_regs, size};
            ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov) < 0) {
        return -errno;
    }
    return 0;
}

int ptrace_set_gp_regs_compat(int pid, const void *user_regs, size_t size) {
    if (iovec iov = {const_cast<void *>(user_regs), size};
            ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) < 0) {
        return -errno;
    }
    return 0;
}

}
