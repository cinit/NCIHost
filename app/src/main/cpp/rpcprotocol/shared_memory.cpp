//
// Created by kinit on 2021-06-24.
//

#include "shared_memory.h"
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/memfd.h>
#include <cerrno>
#include <execution>
#include <cstring>

#define ASHMEM_NAME_DEF "dev/ashmem"
#define ASHMEM_NOT_PURGED 0
#define ASHMEM_WAS_PURGED 1
#define ASHMEM_IS_UNPINNED 0
#define ASHMEM_IS_PINNED 1
struct ashmem_pin {
    __u32 offset;
    __u32 len;
};

#define __ASHMEMIOC 0x77
#define ASHMEM_SET_NAME _IOW(__ASHMEMIOC, 1, char[ASHMEM_NAME_LEN])
#define ASHMEM_GET_NAME _IOR(__ASHMEMIOC, 2, char[ASHMEM_NAME_LEN])
#define ASHMEM_SET_SIZE _IOW(__ASHMEMIOC, 3, size_t)
#define ASHMEM_GET_SIZE _IO(__ASHMEMIOC, 4)
#define ASHMEM_SET_PROT_MASK _IOW(__ASHMEMIOC, 5, unsigned long)
#define ASHMEM_GET_PROT_MASK _IO(__ASHMEMIOC, 6)
#define ASHMEM_PIN _IOW(__ASHMEMIOC, 7, struct ashmem_pin)
#define ASHMEM_UNPIN _IOW(__ASHMEMIOC, 8, struct ashmem_pin)
#define ASHMEM_GET_PIN_STATUS _IO(__ASHMEMIOC, 9)
#define ASHMEM_PURGE_ALL_CACHES _IO(__ASHMEMIOC, 10)
#define ASHMEM_NAME_LEN 256

static int memfd_create_region_post_q(const char *name, size_t size) {
    // This code needs to build on old API levels, so we can't use the libc
    // wrapper.
    int fd(syscall(__NR_memfd_create, name, MFD_CLOEXEC | MFD_ALLOW_SEALING));
    if (fd == -1) {
        return -1;
    }
    if (ftruncate(fd, size) == -1) {
        return -1;
    }
    return fd;
}

static bool testMemfdSupport() {
    int fd = syscall(__NR_memfd_create, "test_support_memfd", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd == -1) {
        return false;
    }
    if (fcntl(fd, F_ADD_SEALS, F_SEAL_FUTURE_WRITE) == -1) {
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

bool has_memfd_support() {
    /*
     * memfd is available when Build.SDK >= Q
     * /dev/ashmem is no longer accessible when SDK >= Q
     */
    static bool memfd_supported = testMemfdSupport();
    return memfd_supported;
}


/* logistics of getting file descriptor for ashmem */
static int ashmem_open_legacy() {
    int fd = TEMP_FAILURE_RETRY(open("/dev/ashmem", O_RDWR | O_CLOEXEC));
    struct stat st = {};
    int ret = TEMP_FAILURE_RETRY(fstat(fd, &st));
    if (ret < 0) {
        int save_errno = errno;
        close(fd);
        errno = save_errno;
        return ret;
    }
    if (!S_ISCHR(st.st_mode) || !st.st_rdev) {
        close(fd);
        errno = ENOTTY;
        return -1;
    }
    return fd;
}

/*
 * ashmem_create_region - creates a new ashmem region and returns the file
 * descriptor, or <0 on error
 *
 * `name' is an optional label to give the region (visible in /proc/pid/maps)
 * `size' is the size of the region, in page-aligned bytes
 */
int ashmem_create_region(const char *name, size_t size) {
    int ret, save_errno;
    if (has_memfd_support()) {
        return memfd_create_region_post_q(name ? name : "none", size);
    }
    int fd = ashmem_open_legacy();
    if (fd < 0) {
        return fd;
    }
    if (name) {
        char buf[ASHMEM_NAME_LEN] = {0};
        strncpy(buf, name, sizeof(buf));
        ret = TEMP_FAILURE_RETRY(ioctl(fd, ASHMEM_SET_NAME, buf));
        if (ret < 0) {
            goto L_ERROR;
        }
    }
    ret = TEMP_FAILURE_RETRY(ioctl(fd, ASHMEM_SET_SIZE, size));
    if (ret < 0) {
        goto L_ERROR;
    }
    return fd;
    L_ERROR:
    save_errno = errno;
    close(fd);
    errno = save_errno;
    return ret;
}
