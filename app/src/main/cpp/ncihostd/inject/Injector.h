// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-10-27.
//

#ifndef NCI_HOST_NATIVES_INJECTOR_H
#define NCI_HOST_NATIVES_INJECTOR_H

#include <array>
#include <string>
#include <string_view>

#include "../../rpcprotocol/utils/HashMap.h"
#include "../../rpcprotocol/log/SessionLog.h"
#include "../elfsym/ProcessView.h"

namespace inject {

class Injector {
private:
    SessionLog *mLog = nullptr;
    int mTargetThreadId = 0;
    int mTargetProcessId = 0;
    int mArchitecture = 0;
    elfsym::ProcessView mProcView;
    uintptr_t mErrnoTlsAddress = 0;
    HashMap<std::string, uintptr_t> mRemoteLibcProc;

public:
    Injector() = default;

    Injector(const Injector &) = delete;

    Injector &operator=(const Injector &o) = delete;

    void setSessionLog(SessionLog *log);

    [[nodiscard]]
    int selectTargetThread(int pid, int tid);

    [[nodiscard]]
    bool isTargetSupported() const noexcept;

    [[nodiscard]]
    int attachTargetProcess();

    [[nodiscard]]
    elfsym::ProcessView getProcessView() const;

    [[nodiscard]]
    int getPointerSize() const noexcept;

    /**
     * Get the SEContext of the main executable of the target process.
     * @param context the result SEContext
     * @return 0 on success, -errno on error
     */
    [[nodiscard]]
    int getMainExecutableSEContext(std::string *context) const;

    int refreshRemoteModules();

    /**
     * Get the errno for the current thread.
     * @param result the result errno
     * @return 0 on success, -errno on error
     */
    [[nodiscard]]
    int getErrnoTls(int *result);

    /**
     * Get the address of the specified module in the remote process.
     * @param moduleName the name of the module, eg. "libc.so"
     * @return the address of the module in the remote process, or 0 if not found
     */
    [[nodiscard]]
    uintptr_t getModuleBaseAddress(std::string_view moduleName) const;

    /**
     * Get the path of the specified module in the remote process.
     * @param moduleName the name of the module, eg. "libc.so"
     * @return the absolute path of the module, or "" if not found
     */
    [[nodiscard]]
    std::string getModulePath(std::string_view moduleName) const;

    [[nodiscard]]
    int getRemoteLibcSymAddress(uintptr_t *pAddr, const char *symbol);

    [[nodiscard]]
    int getRemoteDynSymAddress(uintptr_t *pAddr, const char *soname, const char *symbol) const;

    /**
     * send a fd to target process by SCM_RIGHTS over Unix Domain Socket
     * @param localSock local socket fd
     * @param remoteSock remote socket fd
     * @param sendFd fd to send
     * @return remote fd on success, -errno on failure
     */
    [[nodiscard]]
    int sendFileDescriptor(int localSock, int remoteSock, int sendFd);

    /**
     * create a pair of Unix Domain Socket
     * @param self (out) fd in current process
     * @param that (out) fd in target process
     * @return 0 on success, -errno on failure
     */
    [[nodiscard]]
    int establishUnixDomainSocket(int *self, int *that);

    /**
     * Close a fd in the target process
     * @param fd the fd to close of the target process
     * @return 0 on success, -errno on failure
     */
    int closeRemoteFileDescriptor(int fd);

    [[nodiscard]]
    int allocateRemoteMemory(uintptr_t *remoteAddr, size_t size);

    [[nodiscard]]
    int writeRemoteMemory(uintptr_t remoteAddr, const void *buffer, size_t size);

    [[nodiscard]]
    int allocateCopyToRemoteMemory(uintptr_t *remoteAddr, const void *buffer, size_t size);

    [[nodiscard]]
    int readRemoteMemory(uintptr_t remoteAddr, void *buffer, size_t size);

    [[nodiscard]]
    int callRemoteProcedure(uintptr_t proc, uintptr_t *pRetval,
                            const std::array<uintptr_t, 4> &args, int timeout = 1000);

    [[nodiscard]]
    int remoteLoadLibraryFormFd(const char *soname, int remoteFd);

    /**
     * read C-style string (char*) from target process
     * @param result (out) string container
     * @param address remote address of char[]
     * @return string length if success, -errno if fail
     */
    [[nodiscard]]
    int readRemoteString(std::string *str, uintptr_t address);

    int freeRemoteMemory(uintptr_t addr);

    void detach();
};

}

#endif //NCI_HOST_NATIVES_INJECTOR_H
