//
// Created by kinit on 2021-05-14.
//

#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <linux/limits.h>

#include <cstdio>
#include <cerrno>
#include <cstddef>
#include <algorithm>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include "common.h"
#include "daemon.h"

using namespace std;

typedef struct {
    uint32_t pid;
    char name[108];
} IpcSocketMeta;

static_assert(sizeof(IpcSocketMeta) == 112, "IpcSocketMeta size error");

void startRssWatchdog();

bool doConnect(IpcSocketMeta &target, int uid) {
    sockaddr_un targetAddr = {};
    int clientFd;
    if ((clientFd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("client socket error");
        return false;
    }
    targetAddr.sun_family = AF_UNIX;
    memcpy(targetAddr.sun_path, target.name, 108);
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
    // TODO: recv from cmsg SCM_RIGHTS

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
    startRssWatchdog();
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

static void *rss_watchdog_procedure(void *) {
    using std::ios_base;
    using std::ifstream;
    using std::string;
    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
    while (1) {
        ifstream stat_stream("/proc/self/stat", ios_base::in);
        // dummy vars for leading entries in stat that we don't care about
        string pid, comm, state, ppid, pgrp, session, tty_nr;
        string tpgid, flags, minflt, cminflt, majflt, cmajflt;
        string utime, stime, cutime, cstime, priority, nice;
        string O, itrealvalue, starttime;
        // the two fields we want
        uint32_t vsize;
        uint32_t rss;
        stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
                    >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
                    >> utime >> stime >> cutime >> cstime >> priority >> nice
                    >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest
        stat_stream.close();
        int resident_set = rss * page_size_kb;
        printf("rss = %dK\n", resident_set);
        if (resident_set > 64 * 1024) {
            printf("kill process\n");
            _exit(3);
        }
        usleep(30000);
    }
}

void startRssWatchdog() {
    pthread_t tid;
    pthread_create(&tid, nullptr, &rss_watchdog_procedure, nullptr);
}
