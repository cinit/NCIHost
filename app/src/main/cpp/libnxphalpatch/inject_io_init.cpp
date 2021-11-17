//
// Created by kinit on 2021-10-17.
//
#include <sys/unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdio>

#include "inject_io_init.h"
#include "daemon_ipc.h"
#include "hook_proc_symbols.h"

static InjectInitInfo sInitInfo;
static volatile bool sInitialized = false;
static volatile int gConnFd = -1;

extern "C" int NxpHalPatchInitInternal(int fd, const void *pInfo, int size) {
    {
        char buf[64] = {};
        snprintf(buf, 64, "/proc/self/fd/%d", fd);
        if (access(buf, F_OK) != 0) {
            return -EBADFD;
        }
    }
    if (pInfo == nullptr) {
        return -EINVAL;
    }
    if (size != sizeof(InjectInitInfo)) {
        return -ENOTSUP;
    }
    gConnFd = fd;
    if (sInitialized) {
        return 0;
    }
    memcpy(&sInitInfo, pInfo, sizeof(InjectInitInfo));
    if (int err = hook_sym_init(&sInitInfo.originHookProcedure); err != 0) {
        return err;
    }
    sInitialized = true;
    return 0;
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
