//
// Created by kinit on 2021-10-20.
//

#ifndef NCI_HOST_NATIVES_TRACER_CALL_ASM_H
#define NCI_HOST_NATIVES_TRACER_CALL_ASM_H

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

EXTERN_C __attribute__((noinline))
void *asm_tracer_call(int cmd, void *args);

#undef EXTERN_C

#endif //NCI_HOST_NATIVES_TRACER_CALL_ASM_H
