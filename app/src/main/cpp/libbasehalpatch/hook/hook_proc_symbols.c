//
// Created by kinit on 2021-10-22.
//

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/mman.h>

#include "hook_proc_symbols.h"
#include "intercept_callbacks.h"

#define EXPORT __attribute__((visibility("default")))

static int (*pf_orig_open_2)(const char *pathname, int flags) = NULL;

static int (*pf_orig_open)(const char *pathname, int flags, ...) = NULL;

static ssize_t (*pf_orig_read_chk)(int fd, void *buf, size_t count, size_t buf_size) = NULL;

static ssize_t (*pf_orig_read)(int fd, void *buf, size_t nbytes) = NULL;

static ssize_t (*pf_orig_write_chk)(int fd, const void *buf, size_t count, size_t buf_size) = NULL;

static ssize_t (*pf_orig_write)(int fd, const void *buf, size_t nbytes) = NULL;

static int (*pf_orig_close)(int fd) = NULL;

static int (*pf_orig_ioctl)(int fd, unsigned long int request, ...) = NULL;

static int (*pf_orig_select)(int nfds, void *readfds, void *writefds, void *exceptfds, void *timeout) = NULL;

static int gsIsInitialized = 0;

static void *hookPlt(void *base, uint32_t off, void *hookProc, void *libc, const char *sym_name) {
    if (base == NULL || off == 0) {
        return NULL;
    }
    size_t addr = ((size_t) base) + off;
    void **p = (void **) addr;
    void *orig_ptr = NULL;
    orig_ptr = *p;
    if (orig_ptr == hookProc) {
        // already hooked?
        return dlsym(libc, sym_name);
    }
    void *aligned_ptr = (void *) (((size_t) p) & ~(size_t) (0x1000 - 1));
    mprotect(aligned_ptr, 0x1000, PROT_WRITE | PROT_READ);
    *p = hookProc;
    mprotect(aligned_ptr, 0x1000, PROT_READ);
    __builtin___clear_cache(aligned_ptr, (void *) ((size_t) aligned_ptr + 0x1000U));
    return orig_ptr;
}

int hook_sym_init(const struct OriginHookProcedure *op) {
    if (gsIsInitialized != 0) {
        return 0;
    }
    if (op == NULL) {
        return -EINVAL;
    }
    if (op->struct_size != sizeof(struct OriginHookProcedure)) {
        return -EINVAL;
    }
    void *libc = dlopen("libc.so", RTLD_NOW | RTLD_NOLOAD);
    if (libc == NULL) {
        // should not happen, we have DT_NEEDED
        return -EFAULT;
    }
    void *base = (void *) (op->target_base);
    if (base == NULL) {
        return -EINVAL;
    }
    if (op->off_plt_open_2 != 0) {
        pf_orig_open_2 = hookPlt(base, op->off_plt_open_2, &hook_proc_open_2, libc, "__open_2");
        if (pf_orig_open_2 == NULL) {
            return -EFAULT;
        }
    }
    if (op->off_plt_open != 0) {
        pf_orig_open = hookPlt(base, op->off_plt_open, &hook_proc_open, libc, "open");
        if (pf_orig_open == NULL) {
            return -EFAULT;
        }
    }
    if (op->off_plt_read_chk != 0) {
        pf_orig_read_chk = hookPlt(base, op->off_plt_read_chk, &hook_proc_read_chk, libc, "__read_chk");
        if (pf_orig_read_chk == NULL) {
            return -EFAULT;
        }
    }
    if (op->off_plt_read != 0) {
        pf_orig_read = hookPlt(base, op->off_plt_read, &hook_proc_read, libc, "read");
        if (pf_orig_read == NULL) {
            return -EFAULT;
        }
    }
    if (op->off_plt_write_chk != 0) {
        pf_orig_write_chk = hookPlt(base, op->off_plt_write_chk, &hook_proc_write_chk, libc, "__write_chk");
        if (pf_orig_write_chk == NULL) {
            return -EFAULT;
        }
    }
    if (op->off_plt_write != 0) {
        pf_orig_write = hookPlt(base, op->off_plt_write, &hook_proc_write, libc, "write");
        if (pf_orig_write == NULL) {
            return -EFAULT;
        }
    }
    if (op->off_plt_close != 0) {
        pf_orig_close = hookPlt(base, op->off_plt_close, &hook_proc_close, libc, "close");
        if (pf_orig_close == NULL) {
            return -EFAULT;
        }
    }
    if (op->off_plt_ioctl != 0) {
        pf_orig_ioctl = hookPlt(base, op->off_plt_ioctl, &hook_proc_ioctl, libc, "ioctl");
        if (pf_orig_ioctl == NULL) {
            return -EFAULT;
        }
    }
    if (op->off_plt_select != 0) {
        pf_orig_select = hookPlt(base, op->off_plt_select, &hook_proc_select, libc, "select");
        if (pf_orig_select == NULL) {
            return -EFAULT;
        }
    }
    gsIsInitialized = 1;
    return 0;
}

EXPORT int hook_proc_open_2(const char *pathname, int flags) {
    if (pf_orig_open_2 == NULL) {
        abort();
    }
    int result = pf_orig_open_2(pathname, flags);
    int err = errno;
    invokeOpenResultCallback(result < 0 ? -err : result, pathname, flags, 0);
    errno = err;
    return result;
}

EXPORT int hook_proc_open(const char *pathname, int flags, uint32_t mode) {
    if (pf_orig_open == NULL) {
        abort();
    }
    int result = pf_orig_open(pathname, flags, mode);
    int err = errno;
    invokeOpenResultCallback(result < 0 ? -err : result, pathname, flags, mode);
    errno = err;
    return result;
}

EXPORT ssize_t hook_proc_read_chk(int fd, void *buf, size_t count, size_t buf_size) {
    if (pf_orig_read_chk == NULL) {
        abort();
    }
    ssize_t result = pf_orig_read_chk(fd, buf, count, buf_size);
    int err = errno;
    invokeReadResultCallback(result < 0 ? -err : result, fd, buf, count);
    errno = err;
    return result;
}

EXPORT ssize_t hook_proc_read(int fd, void *buf, size_t nbytes) {
    if (pf_orig_read == NULL) {
        abort();
    }
    ssize_t result = pf_orig_read(fd, buf, nbytes);
    int err = errno;
    invokeReadResultCallback(result < 0 ? -err : result, fd, buf, nbytes);
    errno = err;
    return result;
}

EXPORT ssize_t hook_proc_write_chk(int fd, const void *buf, size_t count, size_t buf_size) {
    if (pf_orig_write_chk == NULL) {
        abort();
    }
    ssize_t result = pf_orig_write_chk(fd, buf, count, buf_size);
    int err = errno;
    invokeWriteResultCallback(result < 0 ? -err : result, fd, buf, count);
    errno = err;
    return result;
}

EXPORT ssize_t hook_proc_write(int fd, const void *buf, size_t nbytes) {
    if (pf_orig_write == NULL) {
        abort();
    }
    ssize_t result = pf_orig_write(fd, buf, nbytes);
    int err = errno;
    invokeWriteResultCallback(result < 0 ? -err : result, fd, buf, nbytes);
    errno = err;
    return result;
}

EXPORT int hook_proc_close(int fd) {
    if (pf_orig_close == NULL) {
        abort();
    }
    int result = pf_orig_close(fd);
    int err = errno;
    invokeCloseResultCallback(result < 0 ? -err : result, fd);
    errno = err;
    return result;
}

EXPORT int hook_proc_ioctl(int fd, unsigned long int request, unsigned long arg) {
    if (pf_orig_ioctl == NULL) {
        abort();
    }
    int result = pf_orig_ioctl(fd, request, arg);
    int err = errno;
    invokeIoctlResultCallback(result == -1 ? -err : result, fd, request, arg);
    errno = err;
    return result;
}

EXPORT int hook_proc_select(int nfds, void *readfds, void *writefds, void *exceptfds, void *timeout) {
    if (pf_orig_select == NULL) {
        abort();
    }
    int result = pf_orig_select(nfds, readfds, writefds, exceptfds, timeout);
    int err = errno;
    invokeSelectResultCallback(result < 0 ? -err : result, nfds, readfds, writefds, exceptfds, timeout);
    errno = err;
    return result;
}
