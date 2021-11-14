// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-10-27.
//

#include <unistd.h>
#include <sys/ptrace.h>
#include <string>
#include <cerrno>
#include <regex>

#include "../../rpcprotocol/utils/FileMemMap.h"
#include "../elfsym/ElfView.h"
#include "arch/ptrace_inject_utils.h"

#include "so_injection.h"

using namespace inject;

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
        // exec
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
    FileMemMap libcMap;
    if (int err;(err = libcMap.mapFilePath(soPath.c_str())) != 0) {
        if (mLog) {
            mLog->error("map " + std::string(soname) + " failed " + std::to_string(err) + ", path=" + soPath);
        }
        return err;
    }
    elfsym::ElfView libcView;
    libcView.attachFileMemMapping({libcMap.getAddress(), libcMap.getLength()});
    int relaAddr = libcView.getSymbolAddress(symbol);
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

/**
 * I. inject & connect
 *    1. trap
 *    2. __errno_location()L
 *    3. socket(III)I
 *    4. malloc(L)L
 *    5. bind(ILI)I
 *    6. listen(II)I
 *    7. fcntl(III)I
 *    8. accept(ILL)I
 *    9. recvmsg(ILI)L
 *    10. android_dlopen_ext(LIL)L, check mmap syscall here
 *    11. <daemon: invoke inject_init with fd>
 * II. reconnect
 *    1. write memory at inject_conn_buffer
 *    2. signal SIGUSR1
 *    3. <daemon: wait for conn 100ms, SEQ_PACKET connect>
 *    4. <both: check credential>
 *    5. <daemon: send pid:I, hwsvc: recv pid:I>
 */
