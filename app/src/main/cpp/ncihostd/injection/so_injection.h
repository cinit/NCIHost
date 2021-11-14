// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-10-27.
//

#ifndef NCI_HOST_NATIVES_SO_INJECTION_H
#define NCI_HOST_NATIVES_SO_INJECTION_H

#include <array>

#include "../../rpcprotocol/utils/HashMap.h"
#include "../../rpcprotocol/log/SessionLog.h"
#include "../elfsym/ProcessView.h"

namespace inject {

class Injection {
private:
    SessionLog *mLog = nullptr;
    int mTargetPid = 0;
    int mArchitecture = 0;
    elfsym::ProcessView mProcView;
    uintptr_t mErrnoTlsAddress = 0;
    HashMap<std::string, uintptr_t> mRemoteLibcProc;

public:
    Injection() = default;

    Injection(const Injection &) = delete;

    Injection &operator=(const Injection &o) = delete;

    void setSessionLog(SessionLog *log);

    [[nodiscard]]
    int selectTargetProcess(int pid);

    [[nodiscard]]
    bool isTargetSupported() const noexcept;

    [[nodiscard]]
    int attachTargetProcess();

    [[nodiscard]]
    elfsym::ProcessView getProcessView() const;

    [[nodiscard]]
    int getErrnoTls(int *result);

    [[nodiscard]]
    int getRemoteLibcSymAddress(uintptr_t *pAddr, const char *symbol);

    [[nodiscard]]
    int getRemoteDynSymAddress(uintptr_t *pAddr, const char *soname, const char *symbol) const;

    /**
     * dup a fd to target process
     * @param fd fd to send
     * @return remote fd on success, -errno on failure
     */
    [[nodiscard]]
    int sendFileDescriptor(int bridge, int fd);

    /**
     * create a pair of Unix Domain Socket
     * @param self (out) fd in current process
     * @param that (out) fd in target process
     * @return 0 on success, -errno on failure
     */
    [[nodiscard]]
    int establishUnixDomainSocket(int *self, int *that);

    [[nodiscard]]
    int allocateRemoteMemory(uintptr_t *remoteAddr, size_t size);

    [[nodiscard]]
    int writeRemoteMemory(uintptr_t remoteAddr, const void *buffer, size_t size);

    [[nodiscard]]
    int readRemoteMemory(uintptr_t remoteAddr, void *buffer, size_t size);

    [[nodiscard]]
    int callRemoteProcedure(uintptr_t proc, uintptr_t *pRetval,
                            const std::array<uintptr_t, 4> &args, int timeout = 1000);

    [[nodiscard]]
    int remoteLoadLibraryFormFd(int remoteFd);

    [[nodiscard]]
    int freeRemoteMemory(uintptr_t addr);

    void detach();
};

}

#endif //NCI_HOST_NATIVES_SO_INJECTION_H
