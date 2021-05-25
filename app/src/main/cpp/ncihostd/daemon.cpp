//
// Created by kinit on 2021-05-14.
//

#include "unistd.h"
#include <cstdio>
#include <cerrno>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include "fcntl.h"
#include "sys/stat.h"
#include <sys/inotify.h>
#include "sys/un.h"
#include "sys/socket.h"
#include "common.h"
#include "daemon.h"

using namespace std;

typedef struct {
    uint32_t pid;
    char name[108];
} IpcSocketMeta;

static_assert(sizeof(IpcSocketMeta) == 112, "IpcSocketMeta size error");

bool doConnect(IpcSocketMeta &target, int uid) {
    sockaddr_un targetAddr = {};
    int clientFd;
    if ((clientFd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("client socket error");
        return false;
    }
    targetAddr.sun_family = AF_UNIX;
    memcpy(targetAddr.sun_path, target.name, min(108, UNIX_PATH_MAX));
    int size = offsetof(struct sockaddr_un, sun_path) + strlen(targetAddr.sun_path + 1) + 1;
    if (::connect(clientFd, (struct sockaddr *) &targetAddr, size) < 0) {
        perror("connect error");
        close(clientFd);
        return false;
    }
    ucred credentials = {};
    socklen_t ucred_length = sizeof(ucred);
    if (::getsockopt(clientFd, SOL_SOCKET, SO_PEERCRED, &credentials, &ucred_length) < 0) {
        printf("getsockopt SO_PEERCRED error: %s\n", strerror(errno));
        close(clientFd);
        return false;
    }
    if (credentials.uid != uid) {
        printf("Invalid UID %d ,expected %d\n", credentials.uid, uid);
        close(clientFd);
        return false;
    }
    printf("Connected...\n");
    sleep(10);
    close(clientFd);
    printf("disconnected\n");
    return true;
}

void startDaemon(int uid, int ipcFileFd, int inotifyFd) {
    char inotifyBuffer[sizeof(struct inotify_event) + NAME_MAX + 1] = {};
    inotify_event &inotifyEvent = *((inotify_event *) &inotifyBuffer);
    char buf64[64];
    do {
        lseek(ipcFileFd, 0, SEEK_SET);
        IpcSocketMeta meta = {};
        int maxLen = sizeof(IpcSocketMeta);
        int len = 0;
        do {
            int i = read(ipcFileFd, &meta, maxLen - len);
            if (i < 0) {
                break;
            }
            len += i;
        } while (len < maxLen);
        if (len < maxLen) {
            printf("read IpcSocketMeta failed: %s\n", strerror(errno));
            _exit(1);
        }
        printf("try connect: pid=%d, name=@%s\n", meta.pid, meta.name + 1);
        if (doConnect(meta, uid)) {
            printf("loop disconnect from pid=%d, name=@%s\n", meta.pid, meta.name + 1);
        } else {
            printf("connect failed, do loop\n");
        }
        if (read(inotifyFd, &inotifyEvent, sizeof(inotifyBuffer)) < 0) {
            printf("read inotifyEvent error: %s\n", strerror(errno));
            _exit(1);
        }
        printf("try reconnect\n");
        usleep(300000);
    } while (true);
}
