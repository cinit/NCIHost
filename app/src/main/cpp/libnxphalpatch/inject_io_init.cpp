//
// Created by kinit on 2021-10-17.
//
#include <sys/unistd.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/un.h>

#include <cerrno>
#include <cstdint>
#include <cstddef>

#include "inject_io_init.h"
#include "daemon_ipc.h"
#include "hook_proc_symbols.h"

static InjectInitInfo sInitInfo;
//static InjectConnectInfo sConnectionInfo;
static volatile bool sInitialized = false;
static volatile int gConnFd = -1;

extern "C" void NxpHalPatchInit() {
    if (!sInitialized) {
        memset((void *) &sInitialized, 0, sizeof(sInitialized));
//        memset((void *) &sConnectionInfo, 0, sizeof(sConnectionInfo));
        sInitialized = true;
    }
    // step 1, get daemon info
    if (tracer_call(TracerCallId::TRACER_CALL_INIT, &sInitInfo) != nullptr) {
        return;
    }
    if (strlen(sInitInfo.daemonVersion) == 0 || strlen(sInitInfo.socketName) == 0) {
        return;
    }
    int listenFd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (listenFd < 0) {
        tracer_printf("create socket error %d", errno);
    }
    struct sockaddr_un listenAddr = {0};
    listenAddr.sun_family = AF_UNIX;
    size_t len = strlen(sInitInfo.socketName);
    memcpy(listenAddr.sun_path + 1, sInitInfo.socketName, len);
    if (size_t size = offsetof(struct sockaddr_un, sun_path) + len + 1;
            ::bind(listenFd, (struct sockaddr *) &listenAddr, (int) size) < 0) {
        tracer_printf("bind socket error %d", errno);
        close(listenFd);
        return;
    }
    if (listen(listenFd, 1) < 0) {
        tracer_printf("listen socket error %d", errno);
        close(listenFd);
        return;
    }
    if (fcntl(listenFd, F_SETFL, fcntl(listenFd, F_GETFL, 0) | O_NONBLOCK) < 0) {
        tracer_printf("fcntl O_NONBLOCK error %d", errno);
        close(listenFd);
        return;
    }
    if (tracer_call(TracerCallId::TRACER_CALL_CONNECT, nullptr) != nullptr) {
        close(listenFd);
        return;
    }
    sockaddr_un addr = {};
    socklen_t adrlen = 0;
    int connfd = accept(listenFd, reinterpret_cast<sockaddr *>(&addr), &adrlen);
    if (connfd < 0) {
        tracer_printf("accept error %d", errno);
        close(listenFd);
        return;
    }
    close(listenFd);
    ucred credentials = {};
    socklen_t ucred_length = sizeof(ucred);
    if (::getsockopt(connfd, SOL_SOCKET, SO_PEERCRED, &credentials, &ucred_length) < 0) {
        tracer_printf("getsockopt SO_PEERCRED error: %s", strerror(errno));
        close(connfd);
        return;
    }
    if (credentials.uid != 0) {
        tracer_printf("Invalid UID %d ,expected 0", credentials.uid);
        close(connfd);
        return;
    }
    fcntl(listenFd, F_SETFL, fcntl(listenFd, F_GETFL, 0) & ~O_NONBLOCK);
    uint32_t tmp = getpid();
    if (write(connfd, &tmp, 4) < 0) {
        tracer_printf("write error %d", errno);
        close(connfd);
        return;
    }
    if (read(connfd, &tmp, 4) < 0) {
        tracer_printf("read error %d", errno);
        close(connfd);
        return;
    }
    fcntl(connfd, F_SETFL, fcntl(gConnFd, F_GETFL, 0) | O_NONBLOCK);
    OriginHookProcedure info = {sizeof(OriginHookProcedure)};
    if (tracer_call(TracerCallId::TRACER_CALL_HOOK_SYM, &info) != nullptr) {
        close(listenFd);
        close(connfd);
        return;
    }
    hook_sym_init(&info);
    gConnFd = connfd;
    tracer_call(TracerCallId::TRACER_CALL_DONE, nullptr);
}

int getDaemonIpcSocket() {
    return gConnFd;
}

void closeDaemonIpcSocket() {
    if (gConnFd >= 0) {
        close(gConnFd);
        gConnFd = -1;
    }
}
