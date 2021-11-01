// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-10-27.
//

#ifndef NCI_HOST_NATIVES_SO_INJECTION_H
#define NCI_HOST_NATIVES_SO_INJECTION_H

#include "../../rpcprotocol/utils/HashMap.h"
#include "../../rpcprotocol/log/SessionLog.h"
#include "../elfsym/ProcessView.h"

namespace inject {

class Injection {
private:
    SessionLog *mLog = nullptr;
    int mTargetPid = 0;
    elfsym::ProcessView mProcView;
    HashMap<std::string, uintptr_t> mRemoteLibcProc;

public:
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
    int getRemoteLibcSymAddress(uintptr_t *pAddr, const char *symbol);

    [[nodiscard]]
    int sendFileDescriptor();

    [[nodiscard]]
    int allocateRemoteMemory(uintptr_t *remoteAddr, size_t size);

    [[nodiscard]]
    int writeRemoteMemory(uintptr_t remoteAddr, const void *buffer, size_t size);

    [[nodiscard]]
    int readRemoteMemory(uintptr_t remoteAddr, void *buffer, size_t size);

    [[nodiscard]]
    int remoteLoadLibraryFormFd(int remoteFd);

    [[nodiscard]]
    int freeRemoteMemory(uintptr_t addr);

    void detach();
};

}

#endif //NCI_HOST_NATIVES_SO_INJECTION_H
