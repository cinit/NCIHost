//
// Created by kinit on 2021-10-17.
//
#include "tracer_call_asm.h"

__attribute__((naked, noinline))
void *asm_tracer_call(int cmd, void *args) {
#if defined(__aarch64__) || defined(__arm64__) || defined (_M_ARM64)
    // r0 = cmd, x1 = args, x0 = ret
    __asm volatile("nop\n"
                   "brk 1\n"
                   "nop\n"
                   "ret\n");
#elif defined(__arm__) || defined(_M_ARM)
    // r0 = cmd, r1 = args, r0 = ret
    __asm volatile("nop\n"
                   "bkpt 1\n"
                   "nop\n"
                   "nop\n"
                   "bx lr\n");
#elif defined(__x86_64__) || defined(__amd64__)
    // rcx = cmd, rdx = args, rax = ret
    __asm volatile("nop\n"
                   "int3\n"
                   "nop\n"
                   "ret\n");
#elif defined(__x86__) || defined(__i386__) || defined(_M_IX86)
    // ecx = cmd, edx = args, eax = ret
    __asm volatile("nop\n"
                   "int3\n"
                   "nop\n"
                   "ret\n");
#else
#error unable to detect ABI for current build
#endif
}
