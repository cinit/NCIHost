//
// Created by kinit on 2021-11-26.
//

#ifndef NCI_HOST_NATIVES_SYSSERVICEPATCH_H
#define NCI_HOST_NATIVES_SYSSERVICEPATCH_H

#include <cstdint>
#include <string_view>
#include <string>

#include "libbasehalpatch/ipc/daemon_ipc_struct.h"
#include "rpcprotocol/log/DefaultSessionLogImpl.h"
#include "Injector.h"

namespace halpatch {

class SysServicePatch {
public:
    class PltHookTarget {
    public:
        static constexpr uint32_t OPEN = 1u << 1u;
        static constexpr uint32_t CLOSE = 1u << 2u;
        static constexpr uint32_t READ = 1u << 3u;
        static constexpr uint32_t WRITE = 1u << 4u;
        static constexpr uint32_t IOCTL = 1u << 5u;
    };

private:
    int mTargetPid = -1;
    int mSharedObjectFd = -1;
    uintptr_t mPatchBaseAddress = 0;
    std::string mSharedObjectName;
    std::string mInitSymbolName;
    int mLocalSocket = -1;
    int mRemoteSocket = -1;
    inject::Injector mInjector;
    DefaultSessionLogImpl mLogger;

public:
    SysServicePatch() = default;

    ~SysServicePatch();

    SysServicePatch(const SysServicePatch &) = delete;

    SysServicePatch &operator=(const SysServicePatch &) = delete;

    /**
     * init the system service patcher
     * @param soname the SONAME of the patch
     * @param path the path of the patch to inject
     * @return 0 on success, -errno on failure
     * possible errors:
     * -ENOENT: the path of shared object to inject is not found
     */
    [[nodiscard]] int init(std::string_view soname, std::string_view path, std::string_view initSymbolName);

    /**
     * init the system service patcher
     * @param soname the SONAME of the patch
     * @param fd the fd of the patch to inject
     * @return 0 on success, -errno on failure
     * possible errors:
     * -ENOENT: the path of shared object to inject is not found
     */
    [[nodiscard]] int init(std::string_view soname, int fd, std::string_view initSymbolName);

    /**
     * set the target system service.
     * @param pid the target process id.
     * @return 0 on success, -errno on failure.
     * possible errors:
     * -EINVAL: invalid status
     * -ESRCH: process not found.
     * -EACCES: permission denied.
     */
    [[nodiscard]] int setSystemServicePid(int pid);

    /**
     * is the service already loaded the patch shared object?
     * @return true if the patch shared object is already loaded, false otherwise.
     */
    [[nodiscard]] bool isServicePatched() const;

    /**
     * inject the patch shared object to the system service
     * @return a non-negative socket fd on success, -errno on failure.
     * possible errors:
     *
     */
    [[nodiscard]] int patchService();

    /**
     * connect to the target service by init function, return a socket file descriptor to the service.
     * @param localSocket the socket file descriptor of this process
     * @param remoteSocket the socket file descriptor of the target process
     * @return 0 on success, -errno on failure.
     */
    [[nodiscard]] int connectToService(int &localSocket, int &remoteSocket);

    void detach();

    /**
     * Get the injector, in case anyone wants to use it directly.
     * @return the injector reference.
     */
    [[nodiscard]] inline inject::Injector &getInjector() {
        return mInjector;
    }

    /**
     * Calculate the offset of the PLT entry to hook in the target shared object.
     * This function will try to find read/write/ioctl/select syscall PLT entry,
     * at least a read and write syscall entry should be found, otherwise it
     * will be considered as entry not found.
     * @param hookProc output result
     * @param targetSoname the target shared object name
     * @return 0 on success, -errno on failure.
     */
    [[nodiscard]] int getPltHookEntries(OriginHookProcedure &hookProc, std::string_view targetSoname,
                                        uint32_t targetHookEntries) const;
};

}

#endif //NCI_HOST_NATIVES_SYSSERVICEPATCH_H
