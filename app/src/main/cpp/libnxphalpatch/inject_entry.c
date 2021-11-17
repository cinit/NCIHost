#ifndef NCI_HOST_VERSION
#error Please define macro NCI_HOST_VERSION in CMakeLists.txt
#endif

#include <stdio.h>
#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "inject_io_init.h"

__attribute__((used, section("NCI_HOST_VERSION"), visibility("default")))
const char g_nci_host_version[] = NCI_HOST_VERSION;

// called by daemon with ptrace
__attribute__((noinline, visibility("default")))
void *nxp_hal_patch_inject_init(int fd, const void *info, int size, const void *_unused) {
    (void) g_nci_host_version;
    return (void *) (size_t) NxpHalPatchInitInternal(fd, info, size);
}

static int get_tracer_pid() {
    const char *status = "/proc/self/status";
    FILE *fp = fopen(status, "r");
    if (!fp) {
        return -1;
    }
    const char *prefix = "TracerPid:";
    const size_t prefix_len = strlen(prefix);
    const char *str = NULL;
    char *line = NULL;
    size_t n = 0;
    while (getline(&line, &n, fp) > 0) {
        if (strncmp(line, prefix, prefix_len) == 0) {
            str = line + prefix_len;
            break;
        }
    }
    if (!str && !line) {
        fclose(fp);
        return -1;
    }
    int tracer = 0;
    if (str != NULL) {
        for (; isspace(*str); ++str);
        tracer = atoi(str);
    }
    free(line);
    fclose(fp);
    return tracer;
}
