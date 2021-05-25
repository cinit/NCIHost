//
// Created by cinit on 2021-05-14.
//

#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "fcntl.h"
#include "sys/stat.h"
#include "memory.h"
#include "string.h"
#include "signal.h"
#include <sys/inotify.h>
#include <sys/prctl.h>

#include "daemon.h"

#ifndef NCI_HOST_VERSION
#error Please define macro NCI_HOST_VERSION in CMakeList.txt
#endif

__attribute__((used, section("HCI_HOST_VERSION")))
const char g_nci_host_version[] = NCI_HOST_VERSION;

static void printVersionAndExit() {
    puts("NCI Host daemon libncihostd.so");
    puts("Version " NCI_HOST_VERSION);
    _exit(1);
}

void setArgv(char **argv, int argc, const char *shortName, const char *longName) {
    if (shortName) {
        char buf16[16] = {};
        snprintf(buf16, 16, "%s", shortName);
        prctl(PR_SET_NAME, buf16, 0, 0, 0);
    }
    if (longName) {
        int nameLen = strlen(longName);
        int maxLength = strlen(argv[argc - 1]) + (argv[argc - 1] - argv[0]);
        memset(argv[0], 0, maxLength);
        int len = maxLength < nameLen ? maxLength : nameLen;
        memcpy(argv[0], longName, len);
    }
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        printVersionAndExit();
        return 0;
    } else if (argc == 2) {
        printf("libncihostd.so [PID] [PATH]\n");
        return 1;
    } else {
        const char *uidStr = argv[1];
        char *end;
        int uid = strtol(uidStr, &end, 10);
        if (uid <= 0 || *end != '\0') {
            printf("argv[1]: Invalid target UID.\n");
            _exit(1);
        }
        if (getegid() != 0) {
            printf("Root(uid=0) is required.\n");
//            _exit(1);
        }
        char *ipcFilePath = argv[2];
        if (*ipcFilePath != '/') {
            printf("Absolute path required.\n");
            _exit(1);
        }
        int ipcFileFd = open(ipcFilePath, O_RDONLY | O_CLOEXEC);
        if (ipcFilePath < 0) {
            perror("open_ipc");
            _exit(1);
        }
        int inotifyFd = inotify_init1(O_CLOEXEC);
        if (ipcFilePath < 0) {
            perror("inotify_init1");
            _exit(1);
        }
        int ipcWatchFd = inotify_add_watch(inotifyFd, ipcFilePath, IN_MODIFY | IN_CLOSE_WRITE);
        if (ipcWatchFd < 0) {
            perror("inotify_add_watch");
            _exit(1);
        }
        printf("Stub, skip setns /proc/1/ns/mnt\n");
        if (ipcFilePath < 0) {
            perror("inotify_init1");
            _exit(1);
        }
        // TODO: daemonize here...(dup /dev/null)
        char buf[32] = {};
        snprintf(buf, 32, "ncihostd: [uid %d]", uid);
        setArgv(argv, argc, "ncihostd", buf);
        signal(SIGPIPE, SIG_IGN);
        startDaemon(uid, ipcFileFd, inotifyFd);
    }
    return 0;
}

