#ifndef NCI_HOST_VERSION
#error Please define macro NCI_HOST_VERSION in CMakeLists.txt
#endif

#include <pthread.h>

#include "io_procedure.h"

__attribute__((used, section("NCI_HOST_VERSION"), visibility("default")))
const char g_nci_host_version[] = NCI_HOST_VERSION;

// called by daemon with ptrace
__attribute__((used, visibility("default")))
void inject_init(void *p) {
    (void) g_nci_host_version;
    if (p != 0) {
        NxpHalPatchInit(p);
    }
}

__attribute__((noinline))
void *asm_tracer_call(int cmd, void *args);

static int get_tracer_pid() {
    return 1;
}

__attribute__((noinline))
void notify_tracer_init() {
    if (get_tracer_pid() != 0) {
        asm_tracer_call(0, 0);
        // no return
        // <jmp inject_init by tracer>
    }
}

__attribute__((constructor))
void so_main() {
    notify_tracer_init();
}
