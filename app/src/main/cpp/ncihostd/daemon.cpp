//
// Created by kinit on 2021-05-14.
//

#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <linux/limits.h>
#include <linux/netlink.h>
#include <linux/connector.h>
#include <linux/cn_proc.h>

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
#include "../rpcprotocol/Log.h"

#define LOG_TAG "ncihostd"

using namespace std;

typedef struct {
    uint32_t pid;
    char name[108];
} IpcSocketMeta;

static_assert(sizeof(IpcSocketMeta) == 112, "IpcSocketMeta size error");

void startRssWatchdog();

bool waitForInotifyWriteEvent(const char *path) {
    // TODO: handle event if file deleted
    char inotifyBuffer[sizeof(struct inotify_event) + NAME_MAX + 1] = {};
    inotify_event &inotifyEvent = *((inotify_event *) &inotifyBuffer);
    int inotifyFd = inotify_init1(O_CLOEXEC);
    if (inotifyFd < 0) {
        LOGE("inotify_init1: %d, %s", errno, strerror(errno));
        return false;
    }
    if (inotify_add_watch(inotifyFd, path, IN_MODIFY | IN_CLOSE_WRITE | IN_MOVE | IN_DELETE_SELF | IN_DELETE)) {
        LOGE("inotify_add_watch: %d, %s", errno, strerror(errno));
        close(inotifyFd);
        return false;
    }
    while (true) {
        int size = read(inotifyFd, inotifyBuffer, sizeof(inotifyBuffer));
        if (size < 0) {
            if (errno != EINTR) {
                LOGE("wait for inotify error:%d, %s", errno, strerror(errno));
                close(inotifyFd);
                return false;
            } else {
                continue;
            }
        } else {
            // event received
            break;
        }
    }
    close(inotifyFd);
    return access(path, R_OK) == 0;
}

bool doConnect(const IpcSocketMeta &target, int uid) {
    sockaddr_un targetAddr = {};
    int clientFd;
    {
        char buf[108];
        snprintf(buf, 108, "/proc/%d/stat", target.pid);
        if (access(buf, F_OK) != 0) {
            LOGW("process %d does not exists", target.pid);
            return false;
        }
    }
    if ((clientFd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("client socket error");
        return false;
    }
    targetAddr.sun_family = AF_UNIX;
    memcpy(targetAddr.sun_path, target.name, 108);
    size_t size = offsetof(struct sockaddr_un, sun_path) + strlen(targetAddr.sun_path + 1) + 1;
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


timespec gStartTime;

void log_print_handler(Log::Level level, const char *tag, const char *msg) {
    timespec systime;
    clock_gettime(CLOCK_MONOTONIC, &systime);
    uint64_t dsec = systime.tv_sec - gStartTime.tv_sec;
    double reltime = double(dsec) + double(int64_t(systime.tv_nsec) - int64_t(gStartTime.tv_sec)) / 1000000000.0;
    char buf[64];
    snprintf(buf, 64, "%.06f", reltime);
    std::cout << buf << " " << (int) level << " " << tag << " " << msg << std::endl;
    std::cout.flush();
}

extern "C" void startDaemon(int uid, const char *ipcFilePath) {
    clock_gettime(CLOCK_MONOTONIC, &gStartTime);
    Log::setLogHandler(&log_print_handler);
    if (std::string(ipcFilePath).find("/ipc.pid") == std::string::npos) {
        LOGE("INVALID PATH: %s", ipcFilePath);
        return;
    }
    startRssWatchdog();
    while (true) {
        int ipcFileFd = open(ipcFilePath, O_RDONLY);
        if (ipcFileFd < 0) {
            if (errno == EINTR || errno == ENOENT) {
                // ignore, go to wait for inotify
            } else {
                LOGE("open %s failed, %d, %s", ipcFilePath, errno, strerror(errno));
                _exit(1);
            }
        } else {
            // read UDS socket name
            lseek(ipcFileFd, 0, SEEK_SET);
            IpcSocketMeta meta = {};
            constexpr int ipcStructLen = sizeof(IpcSocketMeta);
            ssize_t len = 0;
            do {
                ssize_t i = read(ipcFileFd, &meta, ipcStructLen - len);
                if (i < 0) {
                    break;
                }
                len += i;
            } while (len < ipcStructLen);
            if (len < ipcStructLen) {
                LOGE("read IpcSocketMeta failed: %s", strerror(errno));
                // retry
            } else {
                // do connect
                LOGD("try connect: pid=%d, name=@%s", meta.pid, meta.name + 1);
                if (doConnect(meta, uid)) {
                    LOGI("loop disconnect from pid=%d, name=@%s", meta.pid, meta.name + 1);
                } else {
                    LOGE("connect failed, do loop");
                }
            }
            close(ipcFileFd);
        }
        // wait inotify
        if (!waitForInotifyWriteEvent(ipcFilePath)) {
            LOGE("ipc inotify listen failed... quit.");
            _exit(1);
        }
        LOGV("try reconnect");
        usleep(300000);
    }
}


static void *rss_watchdog_procedure(void *) {
    using std::ios_base;
    using std::ifstream;
    using std::string;
    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
    while (true) {
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
        LOGD("rss = %dK", resident_set);
        if (resident_set > 64 * 1024) {
            LOGE("Rss too large %dK, kill process", resident_set);
            _exit(12);
        }
        usleep(30000);
    }
}

void startRssWatchdog() {
    pthread_t tid;
    pthread_create(&tid, nullptr, &rss_watchdog_procedure, nullptr);
}
