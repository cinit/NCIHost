//
// Created by kinit on 2021-10-17.
//

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tracer_call.h"

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

void tracer_printf(const char *fmt, ...) {
    va_list varg1;
    va_list varg2;
    if (fmt == NULL) {
        return;
    }
    va_start(varg1, fmt);
    va_copy(varg2, varg1);
    int size = vsnprintf(NULL, 0, fmt, varg1);
    va_end(varg1);
    if (size <= 0) {
        return;
    }
    void *buffer = malloc(size);
    if (buffer == NULL) {
        return;
    }
    va_start(varg2, fmt);
    vsnprintf(buffer, size, fmt, varg2);
    va_end(varg2);
    LogInfo info = {strlen(buffer), buffer};
    asm_tracer_call(TRACER_CALL_LOG, &info);
    free(buffer);
}
