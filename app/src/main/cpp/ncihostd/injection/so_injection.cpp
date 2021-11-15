// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-10-27.
//

#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/un.h>
#include <string>
#include <cerrno>
#include <regex>

#include "../../rpcprotocol/utils/FileMemMap.h"
#include "../elfsym/ElfView.h"
#include "arch/ptrace_inject_utils.h"
#include "../../rpcprotocol/utils/Uuid.h"

#include "so_injection.h"

using namespace inject;

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

void Injection::setSessionLog(SessionLog *log) {
    mLog = log;
}

int Injection::selectTargetProcess(int pid) {
    if (mTargetPid != 0) {
        detach();
    }
    if (access(("/proc/" + std::to_string(pid) + "/maps").c_str(), R_OK) != 0) {
        return -errno;
    }
    if (int retval; (retval = mProcView.readProcess(pid)) != 0) {
        return retval;
    }
    mTargetPid = pid;
    return 0;
}

bool Injection::isTargetSupported() const noexcept {
    int s = mProcView.getPointerSize();
    return s != 0 && s <= sizeof(void *);
}

int Injection::attachTargetProcess() {
    if (mTargetPid == 0) {
        if (mLog) { mLog->error("try to attach but pid is null"); }
        return -EINVAL;
    }
    if (int err; (err = mProcView.readProcess(mTargetPid)) != 0) {
        if (mLog) {
            mLog->error("unable to get info for pid " + std::to_string(mTargetPid)
                        + ", err=" + std::to_string(err));
        }
        return err;
    }
    mArchitecture = mProcView.getArchitecture();
    if (::ptrace(PTRACE_ATTACH, mTargetPid, nullptr, 0) < 0) {
        int err = errno;
        if (mLog) {
            mLog->error("unable to attach process " + std::to_string(mTargetPid)
                        + ", errno=" + std::to_string(err));
        }
        return -err;
    }
    if (int err; (err = wait_for_signal(mTargetPid, 1000)) < 0) {
        if (mLog) { mLog->error("waitpid err=" + std::to_string(err)); }
        return err;
    }
    if (mLog) {
        mLog->verbose("attached to pid " + std::to_string(mTargetPid));
    }
    return 0;
}

elfsym::ProcessView Injection::getProcessView() const {
    return mProcView;
}

int Injection::getPointerSize() const noexcept {
    return mProcView.getPointerSize();
}

int Injection::getRemoteLibcSymAddress(uintptr_t *pAddr, const char *symbol) {
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

int Injection::getRemoteDynSymAddress(uintptr_t *pAddr, const char *soname, const char *symbol) const {
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

int Injection::allocateRemoteMemory(uintptr_t *remoteAddr, size_t size) {
    uintptr_t remoteMalloc = 0;
    if (int err; (err = getRemoteLibcSymAddress(&remoteMalloc, "malloc")) != 0) {
        if (mLog) { mLog->error("unable to get remote malloc"); }
        return err;
    }
    if (int result = ptrace_call_procedure(mArchitecture, mTargetPid, remoteMalloc, remoteAddr, {size}) != 0) {
        return result;
    }
    return 0;
}

int Injection::writeRemoteMemory(uintptr_t remoteAddr, const void *buffer, size_t size) {
    return ptrace_write_data(mTargetPid, remoteAddr, buffer, int(size));
}

int Injection::readRemoteMemory(uintptr_t remoteAddr, void *buffer, size_t size) {
    return ptrace_read_data(mTargetPid, remoteAddr, buffer, int(size));
}

int Injection::freeRemoteMemory(uintptr_t addr) {
    uintptr_t remoteFree = 0;
    uintptr_t unused;
    if (int err; (err = getRemoteLibcSymAddress(&remoteFree, "free")) != 0) {
        if (mLog) { mLog->error("unable to get remote free: err=" + std::to_string(err)); }
        return err;
    }
    if (int result = ptrace_call_procedure(mArchitecture, mTargetPid, remoteFree, &unused,
                                           {addr}) != 0) {
        return result;
    }
    return 0;
}

int Injection::getErrnoTls(int *result) {
    if (mErrnoTlsAddress == 0) {
        uintptr_t fp_errno_loc = 0;
        if (getRemoteLibcSymAddress(&fp_errno_loc, "__errno_location") != 0) {
            if (int err; (err = getRemoteLibcSymAddress(&fp_errno_loc, "__errno")) != 0) {
                if (mLog) { mLog->error("unable to find libc.so!__errno(_location)?: err=" + std::to_string(err)); }
                return err;
            }
        }
        uintptr_t errnoAddr = 0;
        if (int err; (err = ptrace_call_procedure(mArchitecture, mTargetPid, fp_errno_loc, &errnoAddr, {})) != 0) {
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

void Injection::detach() {
    ptrace(PTRACE_DETACH, mTargetPid, nullptr, 0);
    mRemoteLibcProc.clear();
    mProcView = {};
    mErrnoTlsAddress = 0;
}

int Injection::callRemoteProcedure(uintptr_t proc, uintptr_t *pRetval,
                                   const std::array<uintptr_t, 4> &args, int timeout) {
    uintptr_t retval = -1;
    if (int err = ptrace_call_procedure(mArchitecture, mTargetPid, proc, &retval, args, timeout) != 0) {
        return err;
    }
    if (pRetval != nullptr) {
        *pRetval = retval;
    }
    return 0;
}

int Injection::sendFileDescriptor(int localSock, int remoteSock, int sendFd) {
    if (localSock < 0 || remoteSock < 0 || sendFd < 0) {
        return -EBADFD;
    }
    for (int fd: {localSock, sendFd}) {
        if (access(("/proc/self/fd/" + std::to_string(fd)).c_str(), F_OK) != 0) {
            return -EBADFD;
        }
    }
    logv(mLog, "try to send fd " + std::to_string(sendFd) + " to pid " + std::to_string(mTargetPid)
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
        uint64_t msg_iov;
        uint64_t msg_iovlen;
        uint64_t msg_control;
        uint64_t msg_controllen;
        int msg_flags;
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

int Injection::establishUnixDomainSocket(int *self, int *that) {
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
    if (credentials.pid != mTargetPid && access(("/proc/" + std::to_string(credentials.pid)
                                                 + "/task/" + std::to_string(mTargetPid)).c_str(), F_OK) != 0) {
        loge(mLog, "invalid socket connection, expected pid/tid " + std::to_string(mTargetPid)
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

int Injection::closeRemoteFileDescriptor(int fd) {
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

/**
 * I. inject & connect
 *    1. trap
 *    2. __errno_location()L
 *    3. socket(III)I
 *    5. bind(ILI)I
 *    6. listen(II)I
 *    7. fcntl(III)I
 *    8. accept(ILL)I
 *    9. recvmsg(ILI)L
 *    10. close(I)V
 *    11. android_dlopen_ext(LIL)L, check mmap syscall here
 *    12. <daemon: invoke inject_init with fd>
 * II. reconnect
 *    1. write memory at inject_conn_buffer
 *    2. signal SIGUSR1
 *    3. <daemon: wait for conn 100ms, SEQ_PACKET connect>
 *    4. <both: check credential>
 *    5. <daemon: send pid:I, hwsvc: recv pid:I>
 */
