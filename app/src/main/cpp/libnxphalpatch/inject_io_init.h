//
// Created by kinit on 2021-10-17.
//

#ifndef NCI_HOST_NATIVES_INJECT_IO_INIT_H
#define NCI_HOST_NATIVES_INJECT_IO_INIT_H

#ifdef __cplusplus
extern "C" void NxpHalPatchInit();
#else
void NxpHalPatchInit();
#endif

#ifdef __cplusplus

int getDaemonIpcSocket();

void closeDaemonIpcSocket();

#endif

#endif //NCI_HOST_NATIVES_INJECT_IO_INIT_H
