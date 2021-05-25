//
// Created by kinit on 2021-05-14.
//

#ifndef NCI_HOST_DAEMON_H
#define NCI_HOST_DAEMON_H

#ifdef __cplusplus

extern "C" void startDaemon(int uid, int ipcFileFd, int inotifyFd);

#else

void startDaemon(int uid, int ipcFileFd, int inotifyFd);

#endif

#endif //NCI_HOST_DAEMON_H
