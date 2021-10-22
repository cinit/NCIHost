//
// Created by kinit on 2021-10-22.
//

#ifndef NCI_HOST_NATIVES_HOOK_PROC_SYMBOLS_H
#define NCI_HOST_NATIVES_HOOK_PROC_SYMBOLS_H

#include <stdint.h>
#include <sys/types.h>

struct OriginHookProcedure {
    uint32_t struct_size;
    uint32_t entry_count;
    uint64_t libc_base;
    uint32_t off_plt_open_2;
    uint32_t off_plt_open;
    uint32_t off_plt_read_chk;
    uint32_t off_plt_read;
    uint32_t off_plt_write_chk;
    uint32_t off_plt_write;
    uint32_t off_plt_close;
    uint32_t off_plt_ioctl;
    uint32_t off_plt_select;
    uint32_t unused32_0;
};
#ifdef __cplusplus
static_assert(sizeof(OriginHookProcedure) == 56);
#endif

#ifdef __cplusplus
extern "C" {
#endif

int hook_sym_init(const OriginHookProcedure *originProc);

int hook_proc_open_2(const char *pathname, int flags);

int hook_proc_open(const char *pathname, int flags, uint32_t mode);

ssize_t hook_proc_read_chk(int fd, void *buf, size_t count, size_t buf_size);

ssize_t hook_proc_read(int fd, void *buf, size_t nbytes);

ssize_t hook_proc_write_chk(int fd, const void *buf, size_t count, size_t buf_size);

ssize_t hook_proc_write(int fd, const void *buf, size_t nbytes);

int hook_proc_close(int fd);

int hook_proc_ioctl(int fd, unsigned long int request, unsigned long arg);

int hook_proc_select(int nfds, void *readfds, void *writefds, void *exceptfds, void *timeout);

#ifdef __cplusplus
}
#endif

#endif //NCI_HOST_NATIVES_HOOK_PROC_SYMBOLS_H
