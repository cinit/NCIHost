//
// Created by kinit on 2021-10-21.
//

#ifndef NCI_HOST_NATIVES_DAEMON_IPC_H
#define NCI_HOST_NATIVES_DAEMON_IPC_H

#include "daemon_ipc_struct.h"
#include "intercept_callbacks.h"

void *tracer_call(TracerCallId cmd, void *args);

extern "C" void tracer_printf(const char *fmt, ...)
__attribute__ ((__format__ (__printf__, 1, 2)));

#endif //NCI_HOST_NATIVES_DAEMON_IPC_H
