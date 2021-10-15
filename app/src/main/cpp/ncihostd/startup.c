//
// Created by cinit on 2021-05-14.
//

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/prctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "daemon.h"

#ifndef NCI_HOST_VERSION
#error Please define macro NCI_HOST_VERSION in CMakeList.txt
#endif

__attribute__((used, section("NCI_HOST_VERSION")))
const char g_nci_host_version[] = NCI_HOST_VERSION;

__attribute__((noreturn)) static void printVersionAndExit() {
    printf("NCI Host daemon libncihostd.so\n");
    printf("Version %s\n", g_nci_host_version);
    _exit(1);
}

void setArgv(char **argv, int argc, const char *shortName, const char *longName) {
    if (shortName) {
        char buf16[16] = {};
        snprintf(buf16, 16, "%s", shortName);
        prctl(PR_SET_NAME, buf16, 0, 0, 0);
    }
    if (longName) {
        size_t nameLen = strlen(longName);
        size_t maxLength = strlen(argv[argc - 1]) + (argv[argc - 1] - argv[0]);
        memset(argv[0], 0, maxLength);
        size_t len = maxLength < nameLen ? maxLength : nameLen;
        memcpy(argv[0], longName, len);
    }
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        printVersionAndExit();
    } else if (argc == 4) {
        char *end;
        int uid = (int) strtol(argv[1], &end, 10);
        if (uid <= 0 || *end != '\0') {
            printf("argv[1]: Invalid target UID.\n");
            _exit(1);
        }
        int pid = (int) strtol(argv[2], &end, 10);
        if (pid <= 0 || *end != '\0') {
            printf("argv[2]: Invalid target PID.\n");
            _exit(1);
        }
        int targetFd = (int) strtol(argv[3], &end, 10);
        if (targetFd <= 0 || *end != '\0') {
            printf("argv[3]: Invalid target FD.\n");
            _exit(1);
        }
        if (getegid() != 0) {
            printf("Root(uid=0) is required.\n");
            // _exit(1);
        }
        char buf[108] = {};
        snprintf(buf, 108, "/proc/%d/fd/%d", pid, targetFd);
        if (access(buf, R_OK) != 0) {
            printf("access %s failed: %d, %s\n", buf, errno, strerror(errno));
            return 1;
        }
        char ipcFilePath[108] = {};
        if (readlink(buf, ipcFilePath, 108) < 0) {
            printf("readlink %s failed: %d, %s\n", buf, errno, strerror(errno));
            return 1;
        }
        if (access(ipcFilePath, R_OK) != 0) {
            printf("target real path access %s failed: %d, %s\n", ipcFilePath, errno, strerror(errno));
            return 1;
        }
        printf("using path: %s\n", ipcFilePath);
        // TODO: daemonize here...(dup /dev/null)
        printf("Stub, skip setns /proc/1/ns/mnt\n");
        char namebuf[32] = {};
        snprintf(namebuf, 32, "ncihostd: [uid %d]", uid);
        setArgv(argv, argc, "ncihostd", namebuf);
        signal(SIGPIPE, SIG_IGN);
        startDaemon(uid, ipcFilePath);
    } else {
        printf("$0 <UID> <PID> <FD>\n");
        return 1;
    }
    return 0;
}

