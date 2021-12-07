//
// Created by kinit on 2021-05-14.
//

#ifndef NCI_HOST_DAEMON_H
#define NCI_HOST_DAEMON_H

#ifdef __cplusplus
extern "C" {
#endif

void startDaemon(int uid, const char *ipcFilePath, int daemonize);

#ifdef __cplusplus
};
#endif

#endif //NCI_HOST_DAEMON_H
