//
// Created by kinit on 2021-10-17.
//
#include <sys/unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstddef>
#include <cctype>
#include <cstdlib>

#include "inject_io_init.h"
#include "ipc_looper.h"

static volatile int gConnFd = -1;

extern "C" int BaseHalPatchInitSocket(int fd) {
    {
        char buf[64] = {};
        snprintf(buf, 64, "/proc/self/fd/%d", fd);
        if (access(buf, F_OK) != 0) {
            return -EBADFD;
        }
    }
    gConnFd = fd;
    return startIpcLooper(fd);
}

int getDaemonIpcSocket() {
    return gConnFd;
}

void closeDaemonIpcSocket() {
    if (gConnFd >= 0) {
        close(gConnFd);
        gConnFd = -1;
    }
}

int get_tracer_pid() {
    const char *status = "/proc/self/status";
    FILE *fp = fopen(status, "r");
    if (!fp) {
        return -1;
    }
    const char *prefix = "TracerPid:";
    const size_t prefix_len = strlen(prefix);
    const char *str = nullptr;
    char *line = nullptr;
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
    if (str != nullptr) {
        for (; isspace(*str); ++str);
        tracer = atoi(str);
    }
    free(line);
    fclose(fp);
    return tracer;
}
