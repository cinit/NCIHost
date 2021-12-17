//
// Created by kinit on 2021-10-21.
//

#ifndef NCI_HOST_NATIVES_IPC_IO_EVENT_H
#define NCI_HOST_NATIVES_IPC_IO_EVENT_H

#include "daemon_ipc_struct.h"
#include "../hook/intercept_callbacks.h"

namespace halpatchhook::callback {

void afterHook_read(ssize_t result, int fd, const void *buffer, size_t size);

void afterHook_write(ssize_t result, int fd, const void *buffer, size_t size);

void afterHook_open(int result, const char *name, int flags, uint32_t mode);

void afterHook_close(int result, int fd);

void afterHook_ioctl(int result, int fd, unsigned long int request, uint64_t arg);

void afterHook_select(int result, int nfds, void *readfds, void *writefds, void *exceptfds, void *timeout);

}

#endif //NCI_HOST_NATIVES_IPC_IO_EVENT_H
