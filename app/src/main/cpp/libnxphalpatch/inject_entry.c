#ifndef NCI_HOST_VERSION
#error Please define macro NCI_HOST_VERSION in CMakeLists.txt
#endif

#include <stddef.h>

#include "../libbasehalpatch/ipc/inject_io_init.h"

__attribute__((used, section("NCI_HOST_VERSION"), visibility("default")))
const char g_nci_host_version[] = NCI_HOST_VERSION;

// called by daemon with ptrace
__attribute__((noinline, visibility("default")))
void *nxp_hal_patch_inject_init(int fd) {
    (void) g_nci_host_version;
    return (void *) (size_t) BaseHalPatchInitSocket(fd);
}
