// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-10-27.
//

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string>
#include <cerrno>

#include "../../rpcprotocol/utils/FileMemMap.h"
#include "../elfsym/ElfView.h"
#include "arch/ptrace_inject_common.h"

#include "so_injection.h"

using namespace inject;


void Injection::setSessionLog(SessionLog *log) {
    mLog = log;
}

int Injection::selectTargetProcess(int pid) {
    if (access((std::string() + "/proc/" + std::to_string(pid) + "/maps").c_str(), R_OK) != 0) {
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
    return 0;
}

elfsym::ProcessView Injection::getProcessView() const {
    return mProcView;
}

int Injection::getRemoteLibcSymAddress(uintptr_t *pAddr, const char *symbol) {
    if (const uintptr_t *cached = mRemoteLibcProc.get(symbol); cached != nullptr) {
        *pAddr = *cached;
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
        if (mLog != nullptr) { mLog->error("unable to get base address of libc.so"); }
        return -ESRCH;
    }
    FileMemMap libcMap;
    if (int err;(err = libcMap.mapFilePath(libcPath.c_str())) != 0) {
        if (mLog) {
            mLog->error(std::string() + ("map libc.so failed " + std::to_string(err) + ", path=" + libcPath));
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

int Injection::allocateRemoteMemory(uintptr_t *remoteAddr, size_t size) {
    uintptr_t remoteMalloc = 0;
    if (int err; (err = getRemoteLibcSymAddress(&remoteMalloc, "malloc")) != 0) {
        if (mLog) { mLog->error("unable to get remote malloc"); }
        return err;
    }

    return 0;
}

int Injection::writeRemoteMemory(uintptr_t remoteAddr, const void *buffer, size_t size) {
    return 0;
}

int Injection::readRemoteMemory(uintptr_t remoteAddr, void *buffer, size_t size) {

    return 0;
}

int Injection::freeRemoteMemory(uintptr_t addr) {
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
