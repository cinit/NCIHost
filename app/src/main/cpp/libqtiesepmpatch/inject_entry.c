#ifndef NCI_HOST_VERSION
#error Please define macro NCI_HOST_VERSION in CMakeLists.txt
#endif

#include <stddef.h>
#include <errno.h>

#include "../libbasehalpatch/ipc/inject_io_init.h"

__attribute__((used, section("NCI_HOST_VERSION"), visibility("default")))
const char g_nci_host_version[] = NCI_HOST_VERSION;

// called by daemon with ptrace
__attribute__((noinline, visibility("default")))
void *qti_esepm_patch_inject_init(int fd) {
    (void) g_nci_host_version;
    if (fd < 0) {
        return (void *) -EINVAL;
    }
    if (initElfHeaderInfo("libqtiesepmpatch.so", &qti_esepm_patch_inject_init) == 0) {
        return (void *) -EBADE;
    }
    return (void *) (size_t) BaseHalPatchInitSocket(fd);
}
