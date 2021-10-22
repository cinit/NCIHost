//
// Created by kinit on 2021-10-22.
//

#include <cerrno>
#include <dlfcn.h>
#include <sys/errno.h>

#include "hook_proc_symbols.h"

#define EXPORT extern "C" __attribute__((visibility("default")))

static int (*pf_orig_open_2)(const char *pathname, int flags) = nullptr;

static int (*pf_orig_open)(const char *pathname, int flags, ...) = nullptr;

static ssize_t (*pf_orig_read_chk)(int fd, void *buf, size_t count, size_t buf_size) = nullptr;

static ssize_t (*pf_orig_read)(const char *pathname, int flags) = nullptr;

static ssize_t (*pf_orig_write_chk)(int fd, const void *buf, size_t count, size_t buf_size) = nullptr;

static ssize_t (*pf_orig_write)(int fd, const void *buf, size_t nbytes) = nullptr;

static int (*pf_orig_close)(int fd) = nullptr;

static int (*pf_orig_ioctl)(int fd, unsigned long int request, ...) = nullptr;

static int (*pf_orig_select)(int nfds, void *readfds, void *writefds, void *exceptfds, void *timeout) = nullptr;

extern "C" int hook_sym_init(const OriginHookProcedure *originProc) {
    if (originProc == nullptr) {
        return EINVAL;
    }
    if (originProc->struct_size != sizeof(OriginHookProcedure)) {
        return EINVAL;
    }
    char *libc_base = (char *) (originProc->libc_base);
    pf_orig_open_2 = (int (*)(const char *, int)) (libc_base + originProc->off_plt_open_2);
    pf_orig_open = (int (*)(const char *, int, ...)) (libc_base + originProc->off_plt_open);
    pf_orig_read_chk = (ssize_t (*)(int, void *, size_t, size_t)) (libc_base + originProc->off_plt_read_chk);
    pf_orig_read = (ssize_t (*)(const char *, int)) (libc_base + originProc->off_plt_read);
    pf_orig_write_chk = (ssize_t (*)(int, const void *, size_t, size_t)) (libc_base + originProc->off_plt_write_chk);
    pf_orig_write = (ssize_t (*)(int, const void *, size_t)) (libc_base + originProc->off_plt_write);
    pf_orig_ioctl = (int (*)(int, unsigned long, ...)) (libc_base + originProc->off_plt_ioctl);
    pf_orig_close = (int (*)(int)) (libc_base + originProc->off_plt_close);
    pf_orig_select = (int (*)(int, void *, void *, void *, void *)) (libc_base + originProc->off_plt_select);
    return 0;
}

EXPORT int hook_proc_open_2(const char *pathname, int flags) {

}

EXPORT int hook_proc_open(const char *pathname, int flags, uint32_t mode);

EXPORT ssize_t hook_proc_read_chk(int fd, void *buf, size_t count, size_t buf_size);

EXPORT ssize_t hook_proc_read(int fd, void *buf, size_t nbytes);

EXPORT ssize_t hook_proc_write_chk(int fd, const void *buf, size_t count, size_t buf_size);

EXPORT ssize_t hook_proc_write(int fd, const void *buf, size_t nbytes);

EXPORT int hook_proc_close(int fd);

EXPORT int hook_proc_ioctl(int fd, unsigned long int request, unsigned long arg);

EXPORT int hook_proc_select(int nfds, void *readfds, void *writefds, void *exceptfds, void *timeout);
