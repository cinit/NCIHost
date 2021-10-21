//
// Created by kinit on 2021-10-21.
//

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "daemon_ipc.h"
#include "tracer_call_asm.h"

void *tracer_call(TracerCallId cmd, void *args) {
    return asm_tracer_call(static_cast<int>(cmd), args);
}

extern "C"
void tracer_printf(const char *fmt, ...) {
    va_list varg1;
    va_list varg2;
    if (fmt == nullptr) {
        return;
    }
    va_start(varg1, fmt);
    va_copy(varg2, varg1);
    int size = vsnprintf(nullptr, 0, fmt, varg1);
    va_end(varg1);
    if (size <= 0) {
        return;
    }
    void *buffer = malloc(size);
    if (buffer == nullptr) {
        return;
    }
    va_start(varg2, fmt);
    vsnprintf((char *) buffer, size, fmt, varg2);
    va_end(varg2);
    LogInfo info = {(char *) (buffer), strlen((char *) buffer)};
    tracer_call(TracerCallId::TRACER_CALL_LOG, &info);
    free(buffer);
}
