//
// Created by kinit on 2021-10-17.
//
#include <sys/unistd.h>
#include <sys/mman.h>
#include <cerrno>
#include <dlfcn.h>
#include <cstring>
#include <cstdio>
#include <malloc.h>
#include <cstddef>
#include <cctype>
#include <cstdlib>

#include "inject_io_init.h"
#include "daemon_ipc_struct.h"
#include "ipc_looper.h"

static volatile int gConnFd = -1;

__attribute__((visibility("default")))
halpatch::SharedObjectInfo g_SharedObjectInfo = {};

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

static uintptr_t getSharedLibraryBaseAddress(const char *soname) {
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        return 0;
    }
    char *line = nullptr;
    size_t n = 0;
    while (getline(&line, &n, fp) > 0) {
        if (strstr(line, soname) != nullptr) {
            // found
            char *p = strchr(line, '-');
            if (p) {
                *p = '\0';
                uintptr_t base = strtoul(line, nullptr, 16);
                free(line);
                fclose(fp);
                return base;
            }
            break;
        }
    }
    // not found
    if (line) {
        free(line);
    }
    fclose(fp);
    return 0;
}

extern "C" void *initElfHeaderInfo(const char *soname, void *initFunction) {
    if (soname == nullptr || initFunction == nullptr) {
        return nullptr;
    }
    // get the handle of self, usually soinfo* on Android
    void *handle = dlopen(soname, RTLD_LAZY | RTLD_NOLOAD);
    if (handle == nullptr) {
        return nullptr;
    }
    // keep the handle here
    dlclose(handle);
    uintptr_t base = getSharedLibraryBaseAddress(soname);
    if (base == 0) {
        return nullptr;
    }
    // ELF header has a 16-byte e_ident field
    struct ElfHeaderLike {
        uint8_t used_bytes[9];
        uint8_t padding1;
        uint8_t padding2;
        uint8_t padding3;
        uint32_t offset;
    };
    static_assert(sizeof(ElfHeaderLike) == 16, "sizeof(ElfHeaderLike) != 16");
    auto *header = reinterpret_cast<ElfHeaderLike *> (base);
    uintptr_t offset = uintptr_t(&g_SharedObjectInfo) - base;
    uint8_t checksum = (offset & 0xFFu) ^ ((offset >> 8) & 0xFFu) ^ ((offset >> 16) & 0xFFu) ^ ((offset >> 24) & 0xFFu);
    // check the uint32 overflow
    if (offset > uintptr_t(0xFFFFFFFFu)) {
        return nullptr;
    }
    // set the ELF header writable
    if (mprotect((void *) (base & ~uintptr_t(0xFFFu)), 0x1000, PROT_READ | PROT_WRITE) != 0) {
        return nullptr;
    }
    header->padding3 = checksum;
    header->offset = (uint32_t) offset;
    // restore the ELF header to read-only, just ignore the mprotect result if failed
    mprotect((void *) (base & ~uintptr_t(0xFFFu)), 0x1000, PROT_READ);
    // init the shared object info
    g_SharedObjectInfo.magic = halpatch::SharedObjectInfo::SO_INFO_MAGIC;
    g_SharedObjectInfo.structSize = sizeof(halpatch::SharedObjectInfo);
    g_SharedObjectInfo.initProcAddress = uint64_t(initFunction);
    g_SharedObjectInfo.handle = uint64_t(handle);
    return &g_SharedObjectInfo;
}
