//
// Created by kinit on 2021-10-20.
//

#ifndef NCI_HOST_NATIVES_TRACER_CALL_H
#define NCI_HOST_NATIVES_TRACER_CALL_H

typedef struct {
    char daemonVersion[32];
    char socketName[64];
} InjectInitInfo;

typedef struct {
    char daemonVersion[32];
    char socketName[64];
} InjectConnectInfo;

typedef struct {
    size_t length;
    char *msg;
} LogInfo;

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

EXTERN_C __attribute__((noinline))
void *asm_tracer_call(int cmd, void *args);

EXTERN_C void tracer_printf(const char *fmt, ...)
__attribute__ ((__format__ (__printf__, 1, 2)));

enum {
    TRACER_CALL_LOG = 1,
    TRACER_CALL_INIT = 2,
    TRACER_CALL_CONNECT = 3,
    TRACER_CALL_DETACH = 0xFF,
};

#endif //NCI_HOST_NATIVES_TRACER_CALL_H
