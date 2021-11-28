// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-10-27.
//

#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/un.h>
#include <dlfcn.h>
#include <string>
#include <cerrno>
#include <regex>

#include "../../rpcprotocol/utils/FileMemMap.h"
#include "../elfsym/ElfView.h"
#include "arch/ptrace_inject_utils.h"
#include "../../rpcprotocol/utils/Uuid.h"
#include "../../rpcprotocol/utils/SELinux.h"

#include "Injector.h"

using namespace inject;
using elfsym::ElfView;

static inline void logv(SessionLog *log, std::string_view msg) {
    if (log) {
        log->verbose(msg);
    }
}

static inline void logi(SessionLog *log, std::string_view msg) {
    if (log) {
        log->info(msg);
    }
}

static inline void logw(SessionLog *log, std::string_view msg) {
    if (log) {
        log->warn(msg);
    }
}

static inline void loge(SessionLog *log, std::string_view msg) {
    if (log) {
        log->error(msg);
    }
}

void Injector::setSessionLog(SessionLog *log) {
    mLog = log;
}

int Injector::selectTargetThread(int pid, int tid) {
    if (mTargetThreadId != 0) {
        detach();
    }
    if (access(("/proc/" + std::to_string(pid) + "/task/" + std::to_string(tid)).c_str(), R_OK) != 0) {
        return -ESRCH;
    }
    if (access(("/proc/" + std::to_string(pid) + "/maps").c_str(), R_OK) != 0) {
        return -errno;
    }
    if (int retval; (retval = mProcView.readProcess(pid)) != 0) {
        return retval;
    }
    mTargetThreadId = tid;
    mTargetProcessId = pid;
    return 0;
}

bool Injector::isTargetSupported() const noexcept {
    int s = mProcView.getPointerSize();
    return s != 0 && s <= sizeof(void *);
}

int Injector::attachTargetProcess() {
    if (mTargetThreadId == 0) {
        if (mLog) { mLog->error("try to attach but pid is null"); }
        return -EINVAL;
    }
    if (int err; (err = mProcView.readProcess(mTargetThreadId)) != 0) {
        if (mLog) {
            mLog->error("unable to get info for pid " + std::to_string(mTargetThreadId)
                        + ", err=" + std::to_string(err));
        }
        return err;
    }
    mArchitecture = mProcView.getArchitecture();
    if (::ptrace(PTRACE_ATTACH, mTargetThreadId, nullptr, 0) < 0) {
        int err = errno;
        if (mLog) {
            mLog->error("unable to attach process " + std::to_string(mTargetThreadId)
                        + ", errno=" + std::to_string(err));
        }
        return -err;
    }
    mIsAttached = true;
    if (int err; (err = wait_for_signal(mTargetThreadId, 1000)) < 0) {
        if (mLog) { mLog->error("waitpid err=" + std::to_string(err)); }
        return err;
    }
    if (mLog) {
        mLog->verbose("attached to pid " + std::to_string(mTargetThreadId));
    }
    return 0;
}

elfsym::ProcessView Injector::getProcessView() const {
    return mProcView;
}

int Injector::getPointerSize() const noexcept {
    return mProcView.getPointerSize();
}

int Injector::getRemoteLibcSymAddress(uintptr_t *pAddr, const char *symbol) {
    if (const uintptr_t *cached = mRemoteLibcProc.get(symbol); cached != nullptr) {
        *pAddr = *cached;
        return 0;
    }
    uintptr_t libcBase = 0;
    std::string libcPath;
    for (const auto &m: mProcView.getModules()) {
        if (m.name == "libc.so") {
            libcBase = uintptr_t(m.baseAddress);
            libcPath = m.path;
            break;
        }
    }
    if (libcBase == 0) {
        std::regex libcRegex("libc\\.so\\.[0-9]");
        for (const auto &m: mProcView.getModules()) {
            if (std::regex_match(m.name, libcRegex)) {
                libcBase = uintptr_t(m.baseAddress);
                libcPath = m.path;
                break;
            }
        }
    }
    if (libcBase == 0) {
        std::regex libcRegex("libc-[.0-9]+\\.so");
        for (const auto &m: mProcView.getModules()) {
            if (std::regex_match(m.name, libcRegex)) {
                libcBase = uintptr_t(m.baseAddress);
                libcPath = m.path;
                break;
            }
        }
    }
    if (libcBase == 0) {
        if (mLog != nullptr) { mLog->error("unable to get base address of libc.so"); }
        return -ESRCH;
    }
    FileMemMap libcMap;
    if (int err;(err = libcMap.mapFilePath(libcPath.c_str())) != 0) {
        if (mLog) {
            mLog->error("map libc.so failed " + std::to_string(err) + ", path=" + libcPath);
        }
        return err;
    }
    elfsym::ElfView libcView;
    libcView.attachFileMemMapping({libcMap.getAddress(), libcMap.getLength()});
    int relaAddr = libcView.getSymbolAddress(symbol);
    if (relaAddr != 0) {
        uintptr_t addr = libcBase + relaAddr;
        mRemoteLibcProc.put(symbol, addr);
        *pAddr = addr;
        return 0;
    } else {
        return -ESRCH;
    }
}

int Injector::getRemoteDynSymAddress(uintptr_t *pAddr, const char *soname, const char *symbol) const {
    uintptr_t soBase = 0;
    std::string soPath;
    if (soname == nullptr) {
        // main executable
        soBase = uintptr_t(mProcView.getModules()[0].baseAddress);
        soPath = mProcView.getModules()[0].path;
    } else {
        for (const auto &m: mProcView.getModules()) {
            if (m.name == soname) {
                soBase = uintptr_t(m.baseAddress);
                soPath = m.path;
                break;
            }
        }
        if (soBase == 0) {
            if (mLog != nullptr) { mLog->error("unable to get base address of " + std::string(soname)); }
            return -ESRCH;
        }
    }
    FileMemMap soMap;
    if (int err;(err = soMap.mapFilePath(soPath.c_str())) != 0) {
        if (mLog) {
            mLog->error("map " + std::string(soname) + " failed " + std::to_string(err) + ", path=" + soPath);
        }
        return err;
    }
    elfsym::ElfView soView;
    soView.attachFileMemMapping({soMap.getAddress(), soMap.getLength()});
    int relaAddr = soView.getSymbolAddress(symbol);
    if (relaAddr != 0) {
        uintptr_t addr = soBase + relaAddr;
        *pAddr = addr;
        return 0;
    } else {
        return -ESRCH;
    }
}

int Injector::allocateRemoteMemory(uintptr_t *remoteAddr, size_t size) {
    uintptr_t remoteMalloc = 0;
    if (int err; (err = getRemoteLibcSymAddress(&remoteMalloc, "malloc")) != 0) {
        if (mLog) { mLog->error("unable to get remote malloc"); }
        return err;
    }
    if (int result = ptrace_call_procedure(mArchitecture, mTargetThreadId, remoteMalloc, remoteAddr, {size}) != 0) {
        return result;
    }
    return 0;
}

int Injector::writeRemoteMemory(uintptr_t remoteAddr, const void *buffer, size_t size) {
    return ptrace_write_data(mTargetThreadId, remoteAddr, buffer, int(size));
}

int Injector::readRemoteMemory(uintptr_t remoteAddr, void *buffer, size_t size) {
    return ptrace_read_data(mTargetThreadId, remoteAddr, buffer, int(size));
}

int Injector::allocateCopyToRemoteMemory(uintptr_t *remoteAddr, const void *buffer, size_t size) {
    uintptr_t rbuf = 0;
    if (int err = allocateRemoteMemory(&rbuf, size); err != 0) {
        loge(mLog, "unable to allocate remote memory, err=" + std::to_string(err));
        return -ENOMEM;
    }
    if (int err = writeRemoteMemory(rbuf, buffer, size); err != 0) {
        loge(mLog, "unable to write remote memory, err=" + std::to_string(err));
        freeRemoteMemory(rbuf);
        return -ENOMEM;
    }
    *remoteAddr = rbuf;
    return 0;
}

int Injector::freeRemoteMemory(uintptr_t addr) {
    uintptr_t remoteFree = 0;
    uintptr_t unused;
    if (int err; (err = getRemoteLibcSymAddress(&remoteFree, "free")) != 0) {
        if (mLog) { mLog->error("unable to get remote free: err=" + std::to_string(err)); }
        return err;
    }
    if (int result = ptrace_call_procedure(mArchitecture, mTargetThreadId, remoteFree, &unused,
                                           {addr}) != 0) {
        return result;
    }
    return 0;
}

int Injector::getErrnoTls(int *result) {
    if (mErrnoTlsAddress == 0) {
        uintptr_t fp_errno_loc = 0;
        if (getRemoteLibcSymAddress(&fp_errno_loc, "__errno_location") != 0) {
            if (int err; (err = getRemoteLibcSymAddress(&fp_errno_loc, "__errno")) != 0) {
                if (mLog) { mLog->error("unable to find libc.so!__errno(_location)?: err=" + std::to_string(err)); }
                return err;
            }
        }
        uintptr_t errnoAddr = 0;
        if (int err; (err = ptrace_call_procedure(mArchitecture, mTargetThreadId, fp_errno_loc, &errnoAddr, {})) != 0) {
            if (mLog) { mLog->error("unable to call __errno_location(): err=" + std::to_string(err)); }
            return err;
        }
        mErrnoTlsAddress = errnoAddr;
    }
    int remoteErrno = 0;
    if (int err; (err = readRemoteMemory(mErrnoTlsAddress, &remoteErrno, 4)) != 0) {
        if (mLog) { mLog->error("unable to read int *__errno_location(): err=" + std::to_string(err)); }
        return err;
    }
    *result = remoteErrno;
    return 0;
}

void Injector::detach() {
    ptrace(PTRACE_DETACH, mTargetThreadId, nullptr, 0);
    mIsAttached = false;
    mErrnoTlsAddress = 0;
}

int Injector::callRemoteProcedure(uintptr_t proc, uintptr_t *pRetval,
                                  const std::array<uintptr_t, 4> &args, int timeout) {
    uintptr_t retval = -1;
    if (int err = ptrace_call_procedure(mArchitecture, mTargetThreadId, proc, &retval, args, timeout) != 0) {
        return err;
    }
    if (pRetval != nullptr) {
        *pRetval = retval;
    }
    return 0;
}

int Injector::sendFileDescriptor(int localSock, int remoteSock, int sendFd) {
    if (localSock < 0 || remoteSock < 0 || sendFd < 0) {
        return -EBADFD;
    }
    for (int fd: {localSock, sendFd}) {
        if (access(("/proc/self/fd/" + std::to_string(fd)).c_str(), F_OK) != 0) {
            return -EBADFD;
        }
    }
    logv(mLog, "try to send fd " + std::to_string(sendFd) + " to pid " + std::to_string(mTargetThreadId)
               + " over socket local " + std::to_string(localSock) + " remote " + std::to_string(remoteSock));
    uintptr_t rbuf = 0;
    uintptr_t pf_recvmsg = 0;
    { // init remote
        if (int err;(err = getRemoteLibcSymAddress(&pf_recvmsg, "recvmsg")) != 0) {
            loge(mLog, "unable to dlsym libc.so!recvmsg, err=" + std::to_string(err));
            return err;
        }
        if (int err;(err = allocateRemoteMemory(&rbuf, 256)) != 0) {
            loge(mLog, "unable to allocate remote memory, err=" + std::to_string(err));
            return err;
        }
    }
    { // send fd
        struct msghdr msg = {};
        struct iovec iov = {};
        union {
            struct cmsghdr cmsg;
            char cmsgSpace[CMSG_LEN(sizeof(int))];
        };
        memset(&msg, 0, sizeof(msg));
        memset(&iov, 0, sizeof(iov));
        memset(cmsgSpace, 0, sizeof(cmsgSpace));
        int c = 0;
        iov.iov_base = &c;
        iov.iov_len = 1;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = &cmsg;
        msg.msg_controllen = sizeof(cmsgSpace);
        cmsg.cmsg_len = sizeof(cmsgSpace);
        cmsg.cmsg_level = SOL_SOCKET;
        cmsg.cmsg_type = SCM_RIGHTS;
        *reinterpret_cast<int *>(CMSG_DATA(&cmsg)) = sendFd;
        if (sendmsg(localSock, &msg, 0) < 0) {
            int err = errno;
            loge(mLog, "unable to sendmsg/fd, err=" + std::string(strerror(err)));
            freeRemoteMemory(rbuf);
            return -err;
        }
    }
    // 64+32+16+16
    // init SCM_RIGHTS
    struct iovec32 {
        uint32_t iov_base;
        uint32_t iov_len;
    };
    static_assert(sizeof(iovec32) == 8);
    struct iovec64 {
        uint64_t iov_base;
        uint64_t iov_len;
    };
    static_assert(sizeof(iovec64) == 16);
    struct msghdr32 {
        uint32_t msg_name;
        socklen_t msg_namelen;
        uint32_t msg_iov;
        uint32_t msg_iovlen;
        uint32_t msg_control;
        uint32_t msg_controllen;
        int msg_flags;
    };
    static_assert(sizeof(msghdr32) == 28);
    struct msghdr64 {
        uint64_t msg_name;
        socklen_t msg_namelen;
        int _unused4_0;
        uint64_t msg_iov;
        uint64_t msg_iovlen;
        uint64_t msg_control;
        uint64_t msg_controllen;
        int msg_flags;
        int _unused4_1;
    };
    static_assert(sizeof(msghdr64) == 56);
    struct cmsghdr_fd_32 {
        uint32_t cmsg_len;
        int cmsg_level;
        int cmsg_type;
        int fd;
    };
    static_assert(sizeof(cmsghdr_fd_32) == 16);
    // we only use 20 here
    struct cmsghdr_fd_64 {
        uint64_t cmsg_len;
        int cmsg_level;
        int cmsg_type;
        int fd;
        int _unused4_0;
    };
    static_assert(sizeof(cmsghdr_fd_64) == 24);
    // remote recv fd
    uintptr_t remoteTargetFdAddr = 0;
    char localBuffer[128] = {};
    if (getPointerSize() == 8) {
        using iovec_compat = iovec64;
        using cmsghdr_fd_compat = cmsghdr_fd_64;
        using msghdr_compat = msghdr64;
        using uintptr_compat = uint64_t;
        Rva buf(localBuffer, 128);
        auto *mhdr = buf.at<msghdr_compat>(0);
        auto *iov = buf.at<iovec_compat>(64 + 32);
        mhdr->msg_iov = uintptr_compat(rbuf + 64 + 32);
        mhdr->msg_iovlen = 1;
        mhdr->msg_control = uintptr_compat(rbuf + 64);
        mhdr->msg_controllen = 20;
        iov->iov_base = uintptr_compat(rbuf + 64 + 32 + 16);
        iov->iov_len = 1;
        remoteTargetFdAddr = rbuf + 64 + offsetof(cmsghdr_fd_compat, fd);
    } else {
        using iovec_compat = iovec32;
        using cmsghdr_fd_compat = cmsghdr_fd_32;
        using msghdr_compat = msghdr32;
        using uintptr_compat = uint32_t;
        Rva buf(localBuffer, 128);
        auto *mhdr = buf.at<msghdr_compat>(0);
        auto *iov = buf.at<iovec_compat>(64 + 32);
        mhdr->msg_iov = uintptr_compat(rbuf + 64 + 32);
        mhdr->msg_iovlen = 1;
        mhdr->msg_control = uintptr_compat(rbuf + 64);
        mhdr->msg_controllen = sizeof(cmsghdr_fd_compat);
        iov->iov_base = uintptr_compat(rbuf + 64 + 32 + 16);
        iov->iov_len = 1;
        remoteTargetFdAddr = rbuf + 64 + offsetof(cmsghdr_fd_compat, fd);
    }
    if (int err; (err = writeRemoteMemory(rbuf, localBuffer, 128)) != 0) {
        loge(mLog, "write remote mem err=" + std::to_string(err));
        freeRemoteMemory(rbuf);
        return err;
    }
    uintptr_t retval = -1;
    if (int err; (err = callRemoteProcedure(pf_recvmsg, &retval, {uintptr_t(remoteSock), rbuf, 0})) != 0) {
        loge(mLog, "call remote recvmsg err=" + std::to_string(err));
        freeRemoteMemory(rbuf);
        return err;
    }
    if (int(retval) < 0) {
        int rerr = 0;
        if (int e2;(e2 = getErrnoTls(&rerr)) != 0) {
            loge(mLog, "get remote errno fail, err=" + std::to_string(e2));
            freeRemoteMemory(rbuf);
            return e2;
        } else {
            loge(mLog, "remote recvmsg error, remote errno=" + std::to_string(rerr));
            freeRemoteMemory(rbuf);
            return -rerr;
        }
    }
    int resultFd = -1;
    if (int err; (err = readRemoteMemory(remoteTargetFdAddr, &resultFd, 4)) != 0) {
        loge(mLog, "read remote memory err=" + std::to_string(err));
        return err;
    }
    freeRemoteMemory(rbuf);
    return resultFd;
}

int Injector::establishUnixDomainSocket(int *self, int *that) {
    // init remote proc address
    constexpr int pf_close = 0;
    constexpr int pf_socket = 1;
    constexpr int pf_bind = 2;
    constexpr int pf_listen = 3;
    constexpr int pf_fcntl = 4;
    constexpr int pf_accept = 5;
    struct RemoteFunc {
        const char *name;
        uintptr_t addr;
    };
    std::array<RemoteFunc, 6> remoteFuncs = {
            RemoteFunc{"close", 0},
            RemoteFunc{"socket", 0},
            RemoteFunc{"bind", 0},
            RemoteFunc{"listen", 0},
            RemoteFunc{"fcntl", 0},
            RemoteFunc{"accept", 0},
    };
    for (auto &[name, addr]: remoteFuncs) {
        if (int err; (err = getRemoteLibcSymAddress(&addr, name)) != 0) {
            loge(mLog, "unable to dlsym libc.so!" + std::string(name) + ", err=" + std::to_string(err));
            return err;
        }
    }
    int rerr = 0;
    if (int err; (err = getErrnoTls(&rerr)) != 0) {
        loge(mLog, "unable to read remote errno, err=" + std::to_string(err));
        return err;
    }
    uintptr_t rbuf = 0;
    if (int err; (err = allocateRemoteMemory(&rbuf, 4096)) != 0) {
        loge(mLog, "unable to allocate remote memory, err=" + std::to_string(err));
        return err;
    }
    uintptr_t tmpretval = -1;
    int rsock;
    size_t addrLen;
    struct sockaddr_un listenAddr = {0};
    { // 1. socket(AF_UNIX, SOCK_SEQPACKET, 0)
        if (int err; (err = callRemoteProcedure(remoteFuncs[pf_socket].addr, &tmpretval,
                                                {AF_UNIX, SOCK_SEQPACKET, 0})) != 0) {
            loge(mLog, "unable to call remote socket, err=" + std::to_string(err));
            return err;
        }
        rsock = int(tmpretval);
        if (rsock < 0) {
            (void) getErrnoTls(&rerr);
            loge(mLog, "remote socket failed, remote errno=" + std::to_string(rerr));
            return -rerr;
        }
        // init sockaddr
        listenAddr.sun_family = AF_UNIX;
        std::string uuid = Uuid::randomUuid();
        size_t nameLen = uuid.size();
        memcpy(listenAddr.sun_path + 1, uuid.c_str(), nameLen);
        addrLen = offsetof(struct sockaddr_un, sun_path) + nameLen + 1;
        if (int err; (err = writeRemoteMemory(rbuf, &listenAddr, addrLen)) != 0) {
            loge(mLog, "unable to write remote mem " + std::to_string(rbuf) + ", len=" + std::to_string(addrLen)
                       + ", err=" + std::to_string(err));
            return err;
        }
    }
    { // 2. bind(listenFd, &listenAddr, size)
        tmpretval = -1;
        if (int err; (err = callRemoteProcedure(remoteFuncs[pf_bind].addr, &tmpretval,
                                                {uint32_t(rsock), rbuf, addrLen})) != 0) {
            loge(mLog, "unable to call remote bind, err=" + std::to_string(err));
            return err;
        }
        if (int(tmpretval) < 0) {
            (void) getErrnoTls(&rerr);
            loge(mLog, "remote bind failed, remote errno=" + std::to_string(rerr));
            return -rerr;
        }
    }
    { // 3. listen(listenFd, 1)
        tmpretval = -1;
        if (int err; (err = callRemoteProcedure(remoteFuncs[pf_listen].addr, &tmpretval,
                                                {uint32_t(rsock), 1})) != 0) {
            loge(mLog, "unable to call remote listen, err=" + std::to_string(err));
            return err;
        }
        if (int(tmpretval) < 0) {
            (void) getErrnoTls(&rerr);
            loge(mLog, "remote listen failed, remote errno=" + std::to_string(rerr));
            return -rerr;
        }
    }
    { // 4. fcntl(listenFd, F_SETFL, fcntl(listenFd, F_GETFL, 0) | O_NONBLOCK)
        tmpretval = -1;
        if (int err; (err = callRemoteProcedure(remoteFuncs[pf_fcntl].addr, &tmpretval,
                                                {uint32_t(rsock), F_GETFL, 0})) != 0) {
            loge(mLog, "unable to call remote fcntl, err=" + std::to_string(err));
            return err;
        }
        int rfattr = int(tmpretval);
        if (int err; (err = callRemoteProcedure(remoteFuncs[pf_fcntl].addr, &tmpretval,
                                                {uint32_t(rsock), F_SETFL, uintptr_t(rfattr | O_NONBLOCK)})) != 0) {
            loge(mLog, "unable to call remote fcntl, err=" + std::to_string(err));
            return err;
        }
        if (int(tmpretval) < 0) {
            (void) getErrnoTls(&rerr);
            loge(mLog, "remote fcntl failed, remote errno=" + std::to_string(rerr));
            return -rerr;
        }
    }
    int lConnSock = -1;
    { // active side
        lConnSock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
        fcntl(lConnSock, F_SETFL, fcntl(lConnSock, F_GETFL, 0) | O_NONBLOCK);
        int err = connect(lConnSock, reinterpret_cast<const sockaddr *>(&listenAddr), int(addrLen));
        if (err < 0 && err != EINPROGRESS) {
            loge(mLog, "local connect async failed, errno=" + std::to_string(rerr));
            close(lConnSock);
            return -err;
        }
    }
    int rConnSock = -1;
    { // 6. int connfd = accept(listenFd, &addr, &adrlen)
        tmpretval = -1;
        if (int err; (err = callRemoteProcedure(remoteFuncs[pf_accept].addr, &tmpretval,
                                                {uint32_t(rsock), rbuf + 256, rbuf + 256 + 16})) != 0) {
            loge(mLog, "unable to call remote accept, err=" + std::to_string(err));
            return err;
        }
        rConnSock = int(tmpretval);
        if (rConnSock < 0) {
            (void) getErrnoTls(&rerr);
            loge(mLog, "remote accept failed, remote errno=" + std::to_string(rerr));
            close(lConnSock);
            return -rerr;
        }
    }
    { // close unused fd
        tmpretval = -1;
        if (int err; (err = callRemoteProcedure(remoteFuncs[pf_close].addr, &tmpretval,
                                                {uint32_t(rsock)})) != 0) {
            logw(mLog, "unable to call remote close, err=" + std::to_string(err));
        }
        // free remote memory
        (void) freeRemoteMemory(rbuf);
    }
    if (fcntl(lConnSock, F_SETFL, fcntl(lConnSock, F_GETFL, 0) & ~O_NONBLOCK)) {
        logw(mLog, "unable to unset O_NONBLOCK, err=" + std::to_string(errno));
    }
    ucred credentials = {};
    if (socklen_t ucred_length = sizeof(ucred);
            ::getsockopt(lConnSock, SOL_SOCKET, SO_PEERCRED, &credentials, &ucred_length) < 0) {
        int err = errno;
        loge(mLog, "getsockopt SO_PEERCRED error: %s\n" + std::string(strerror(err)));
        close(lConnSock);
        if (int err2; (err2 = callRemoteProcedure(remoteFuncs[pf_close].addr, &tmpretval,
                                                  {uint32_t(rConnSock)})) != 0) {
            logw(mLog, "unable to call remote close, err=" + std::to_string(err2));
        }
        return err;
    }
    if (credentials.pid != mTargetThreadId && access(("/proc/" + std::to_string(credentials.pid)
                                                      + "/task/" + std::to_string(mTargetThreadId)).c_str(), F_OK) !=
                                              0) {
        loge(mLog, "invalid socket connection, expected pid/tid " + std::to_string(mTargetThreadId)
                   + ", got " + std::to_string(credentials.pid));
        close(lConnSock);
        if (int err2; (err2 = callRemoteProcedure(remoteFuncs[pf_close].addr, &tmpretval,
                                                  {uint32_t(rConnSock)})) != 0) {
            logw(mLog, "unable to call remote close, err=" + std::to_string(err2));
        }
        return -EBUSY;
    }
    *self = lConnSock;
    *that = rConnSock;
    return 0;
}

int Injector::closeRemoteFileDescriptor(int fd) {
    uintptr_t proc = 0;
    if (int err;(err = getRemoteLibcSymAddress(&proc, "close")) != 0) {
        loge(mLog, "unable to dlsym libc.so!close, err=" + std::to_string(err));
        return err;
    }
    uintptr_t retval = -1;
    if (int err2; (err2 = callRemoteProcedure(proc, &retval, {uint32_t(fd)})) != 0) {
        loge(mLog, "unable to call remote close, err=" + std::to_string(err2));
    }
    if (retval != 0) {
        int rerr = 0;
        if (int err; (err = getErrnoTls(&rerr)) != 0) {
            loge(mLog, "get remote errno error, err=" + std::to_string(err));
        }
        loge(mLog, "remote close, remote errno=" + std::to_string(rerr));
        return -rerr;
    }
    return 0;
}

int Injector::remoteLoadLibraryFormFd(std::string_view soname, int remoteFd, std::string *pErrMsg) {
    uintptr_t linkerBase = 0;
    std::string linkerPath;
    uintptr_t libcBase = 0;
    std::string libcPath;
    for (const auto &m: mProcView.getModules()) {
        if (m.name == "linker" || m.name == "linker64") {
            linkerBase = uintptr_t(m.baseAddress);
            linkerPath = m.path;
            continue;
        } else if (m.name == "libc.so") {
            libcBase = uintptr_t(m.baseAddress);
            libcPath = m.path;
            continue;
        }
    }
    if (linkerBase == 0 && libcBase == 0) {
        std::regex libcRegex("libc\\.so\\.[0-9]");
        for (const auto &m: mProcView.getModules()) {
            if (std::regex_match(m.name, libcRegex)) {
                libcBase = uintptr_t(m.baseAddress);
                libcPath = m.path;
                break;
            }
        }
    }
    if (linkerBase == 0 && libcBase == 0) {
        std::regex libcRegex("libc-[.0-9]+\\.so");
        for (const auto &m: mProcView.getModules()) {
            if (std::regex_match(m.name, libcRegex)) {
                libcBase = uintptr_t(m.baseAddress);
                libcPath = m.path;
                break;
            }
        }
    }
    if (libcBase == 0 && linkerBase == 0) {
        loge(mLog, "unable to find linker or libc.so");
        return -EFAULT;
    }
    if (linkerBase != 0) {
        logi(mLog, "use linker as remote dynamic loader");
        FileMemMap linkerMemMap;
        if (int err;(err = linkerMemMap.mapFilePath(linkerPath.c_str())) != 0) {
            if (mLog) {
                mLog->error("map linker failed " + std::to_string(err) + ", path=" + linkerPath);
            }
            return err;
        }
        uintptr_t estimatedCallerAddress = 0;
        if (getRemoteLibcSymAddress(&estimatedCallerAddress, "malloc") != 0) {
            logw(mLog, "unable to dlsym malloc, will use main executable as caller address");
            estimatedCallerAddress = mProcView.getModules()[0].baseAddress + 4096;
        }
        elfsym::ElfView linkerView;
        linkerView.attachFileMemMapping({linkerMemMap.getAddress(), linkerMemMap.getLength()});
        uintptr_t dlopenRelAddr = linkerView.getSymbolAddress("__loader_android_dlopen_ext");
        uintptr_t dlerrorRelAddr = linkerView.getSymbolAddress("__loader_dlerror");
        if (dlopenRelAddr != 0) {
            char localBuf[96] = {};
            constexpr int ANDROID_DLEXT_USE_LIBRARY_FD = 0x10;
            if (getPointerSize() == 8) {
                struct ExtInfo64 {
                    uint64_t flags;
                    uint64_t reserved_addr;
                    uint64_t reserved_size;
                    int relro_fd;
                    int library_fd;
                    uint64_t library_fd_offset;
                    uint64_t library_namespace;
                };
                auto *pExtInfo = reinterpret_cast<struct ExtInfo64 *>(localBuf);
                pExtInfo->flags = ANDROID_DLEXT_USE_LIBRARY_FD;
                pExtInfo->library_fd = remoteFd;
            } else {
                struct ExtInfo32 {
                    uint64_t flags;
                    uint32_t reserved_addr;
                    uint32_t reserved_size;
                    int relro_fd;
                    int library_fd;
                    uint64_t library_fd_offset;
                    uint32_t library_namespace;
                };
                auto *pExtInfo = reinterpret_cast<struct ExtInfo32 *>(localBuf);
                pExtInfo->flags = ANDROID_DLEXT_USE_LIBRARY_FD;
                pExtInfo->library_fd = remoteFd;
            }
            if (!soname.empty()) {
                memcpy(localBuf + 64, soname.data(), std::min(size_t(32), soname.length()));
            }
            uintptr_t pf_dlopen_ext = linkerBase + dlopenRelAddr;
            uintptr_t retval = 0;
            uintptr_t rbuf = 0;
            if (int err;(err = allocateRemoteMemory(&rbuf, 96)) != 0) {
                loge(mLog, "remote malloc failed");
                return err;
            }
            if (int err;(err = writeRemoteMemory(rbuf, localBuf, 96)) != 0) {
                loge(mLog, "write remote memory failed");
                freeRemoteMemory(rbuf);
                return err;
            }
            if (int err; (err = callRemoteProcedure(pf_dlopen_ext, &retval,
                                                    {rbuf + 64, RTLD_NOW, rbuf, estimatedCallerAddress}))) {
                loge(mLog, "call remote __loader_android_dlopen_ext failed, err=" + std::to_string(err));
                freeRemoteMemory(rbuf);
                return err;
            }
            freeRemoteMemory(rbuf);
            // refresh process modules
            refreshRemoteModules();
            if (retval != 0) {
                return 0;
            } else {
                if (dlerrorRelAddr != 0) {
                    if (int err; (err = callRemoteProcedure(linkerBase + dlerrorRelAddr, &retval, {}))) {
                        loge(mLog, "call remote __loader_dlopen&dlerror failed, err=" + std::to_string(err));
                        return err;
                    }
                    std::string errMsg;
                    if (int e2 = readRemoteString(&errMsg, retval); e2 < 0) {
                        loge(mLog, "remote dlopen failed, read __loader_dlerror err=" + std::to_string(e2));
                        return -EFAULT;
                    }
                    if (pErrMsg != nullptr) {
                        *pErrMsg = errMsg;
                    }
                    loge(mLog, "call remote __loader_android_dlopen_ext failed, err=" + errMsg);
                    return -EFAULT;
                } else {
                    loge(mLog, "remote dlopen failed,  no __loader_dlerror available");
                    return -EFAULT;
                }
            }
        } else {
            loge(mLog, "found linker but dlsym __loader_android_dlopen_ext failed");
            return -ENOKEY;
        }
    } else {
        logi(mLog, "use libc.so as remote dynamic loader");
        uintptr_t pf_libc_dlopen_mode = 0;
        if (int err = getRemoteLibcSymAddress(&pf_libc_dlopen_mode, "__libc_dlopen_mode"); err != 0) {
            loge(mLog, "dlsym libc.so!__libc_dlopen_mode failed, err=" + std::to_string(err));
            return err;
        }
        char localBuf[64] = {};
        snprintf(localBuf, 64, "/proc/self/fd/%d", remoteFd);
        uintptr_t rbuf = 0;
        if (int err = allocateCopyToRemoteMemory(&rbuf, localBuf, 64); err != 0) {
            loge(mLog, "unable to allocate remote memory, err=" + std::to_string(err));
            return err;
        }
        uintptr_t retval = 0;
        if (int err = callRemoteProcedure(pf_libc_dlopen_mode, &retval, {rbuf, RTLD_NOW}); err != 0) {
            loge(mLog, "unable to call __libc_dlopen_mode, err=" + std::to_string(err));
            return err;
        }
        freeRemoteMemory(rbuf);
        // refresh process modules
        refreshRemoteModules();
        if (retval != 0) {
            return 0;
        }
        loge(mLog, "__libc_dlopen_mode return null");
        return -EFAULT;
    }
}

int Injector::readRemoteString(std::string *str, uintptr_t address) {
    uintptr_t pos = address & ~(sizeof(void *) - 1);
    uintptr_t startOffset = address - pos;
    if (pos == 0) {
        return -EFAULT;
    }
    std::vector<uintptr_t> vecResult;
    static_assert(sizeof(void *) == sizeof(uintptr_t));
    static_assert(sizeof(void *) == sizeof(long));
    do {
        errno = 0;
        long tmp;
        if ((tmp = ::ptrace(PTRACE_PEEKDATA, mTargetThreadId, pos, 0)) == -1 && errno != 0) {
            return -errno;
        }
        pos += sizeof(void *);
        vecResult.push_back(uintptr_t(tmp));
        if constexpr(sizeof(void *) == 8) {
            uint64_t v = uint64_t(tmp);
            if (((v >> 0) & 0xFFu) == 0 || ((v >> 8) & 0xFFu) == 0 || ((v >> 16) & 0xFFu) == 0
                || ((v >> 24) & 0xFFu) == 0 || ((v >> 32) & 0xFFu) == 0 || ((v >> 40) & 0xFFu) == 0
                || ((v >> 48) & 0xFFu) == 0 || ((v >> 56) & 0xFFu) == 0) {
                break;
            }
        } else {
            uint32_t v = uint32_t(tmp);
            if (((v >> 0) & 0xFFu) == 0 || ((v >> 8) & 0xFFu) == 0
                || ((v >> 16) & 0xFFu) == 0 || ((v >> 24) & 0xFFu) == 0) {
                break;
            }
        }
    } while (true);
    void *start = &vecResult[0];
    size_t length = uintptr_t(memchr(start, 0, vecResult.size() * sizeof(void *))) - uintptr_t(start);
    *str = std::string(reinterpret_cast<const char *>(&vecResult[0]) + startOffset, length - startOffset);
    return int(length - startOffset);
}

int Injector::getMainExecutableSEContext(std::string *context) const {
    if (SELinux::isEnabled()) {
        int ret = SELinux::getFileSEContext(("/proc/" + std::to_string(mTargetProcessId) + "/exe").c_str(), context);
        if (ret < 0) {
            return ret;
        } else {
            return 0;
        }
    } else {
        return -ENOTSUP;
    }
}

int Injector::refreshRemoteModules() {
    return mProcView.readProcess(mTargetProcessId);
}

uintptr_t Injector::getModuleBaseAddress(std::string_view moduleName) const {
    for (const auto &m: mProcView.getModules()) {
        if (m.name == moduleName) {
            return m.baseAddress;
        }
    }
    return 0;
}

std::string Injector::getModulePath(std::string_view moduleName) const {
    for (const auto &m: mProcView.getModules()) {
        if (m.name == moduleName) {
            return m.path;
        }
    }
    return "";
}
