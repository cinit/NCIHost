//
// Created by kinit on 2021-11-26.
//
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

#include "rpcprotocol/utils/SELinux.h"
#include "rpcprotocol/utils/FileMemMap.h"
#include "rpcprotocol/log/Log.h"
#include "libbasehalpatch/ipc/daemon_ipc_struct.h"
#include "../elfsym/ElfView.h"

#include "SysServicePatch.h"

using namespace halpatch;
using namespace elfsym;
using namespace inject;

constexpr const char *const LOG_TAG = "SysServicePatch";

SysServicePatch::~SysServicePatch() {
    detach();
    if (mSharedObjectFd > 0) {
        close(mSharedObjectFd);
    }
}

void SysServicePatch::detach() {
    if (mInjector.isAttached()) {
        mInjector.detach();
    }
}

int SysServicePatch::init(std::string_view soname, int fd, std::string_view initSymbolName) {
    mLogger.setLogTag(LOG_TAG);
    mInjector.setSessionLog(&mLogger);
    mSharedObjectName = soname;
    mInitSymbolName = initSymbolName;
    if (mInitSymbolName.empty() || mSharedObjectName.empty()) {
        return -EINVAL;
    }
    mSharedObjectFd = dup(fd);
    if (mSharedObjectFd < 0) {
        return -errno;
    }
    lseek64(mSharedObjectFd, 0, SEEK_SET);
    return 0;
}

int SysServicePatch::init(std::string_view soname, std::string_view path, std::string_view initSymbolName) {
    mLogger.setLogTag(LOG_TAG);
    mInjector.setSessionLog(&mLogger);
    mSharedObjectName = soname;
    mInitSymbolName = initSymbolName;
    if (mInitSymbolName.empty() || mSharedObjectName.empty()) {
        return -EINVAL;
    }
    mSharedObjectFd = open(std::string(path).c_str(), O_RDONLY);
    if (mSharedObjectFd < 0) {
        return -errno;
    }
    return 0;
}

int SysServicePatch::setSystemServicePid(int pid) {
    if (int err = mInjector.selectTargetThread(pid, pid); err < 0) {
        return err;
    }
    mTargetPid = pid;
    mPatchBaseAddress = mInjector.getModuleBaseAddress(mSharedObjectName);
    return 0;
}

bool SysServicePatch::isServicePatched() const {
    return mPatchBaseAddress != 0;
}

int SysServicePatch::patchService() {
    if (mPatchBaseAddress != 0) {
        // already loaded
        return 0;
    }
    if (int err = mInjector.selectTargetThread(mTargetPid, mTargetPid); err < 0) {
        return err;
    }
    if (SELinux::isEnabled()) {
        // SELinux is enabled, we need to change the context of the shared object
        std::string magiskContext = "u:object_r:magisk_file:s0";
        // TODO: if Magisk is not available, use the same context as the original shared object
        if (int err = SELinux::setFileSEContext(mSharedObjectFd, magiskContext);err < 0) {
            LOGE("Failed to set SELinux context for %s, err=%d", mSharedObjectName.c_str(), err);
            return -err;
        }
    }
    if (!mInjector.isAttached()) {
        if (int err = mInjector.attachTargetProcess(); err != 0) {
            LOGE("Failed to attach to target process: %d", err);
            return err;
        }
    }
    // create socket connection
    if (mLocalSocket < 0 || mRemoteSocket < 0) {
        if (int err = mInjector.establishUnixDomainSocket(&mLocalSocket, &mRemoteSocket); err != 0) {
            LOGE("Failed to establish socket: %d", err);
            return err;
        }
    }
    // send shared object fd to target
    int remoteSoFd = mInjector.sendFileDescriptor(mLocalSocket, mRemoteSocket, mSharedObjectFd);
    if (remoteSoFd < 0) {
        LOGE("Failed to send shared object fd: %d", remoteSoFd);
        return remoteSoFd;
    }
    // call dlopen in target process
    if (int err = mInjector.remoteLoadLibraryFormFd(mSharedObjectName, remoteSoFd, nullptr); err != 0) {
        LOGE("Failed to remote load shared object: %d", err);
        return err;
    }
    mPatchBaseAddress = mInjector.getModuleBaseAddress(mSharedObjectName);
    if (mPatchBaseAddress == 0) {
        LOGE("ERROR: remote reported dlopen success failed to get base address of %s", mSharedObjectName.c_str());
        for (const auto &m: mInjector.getProcessView().getModules()) {
            LOGE("%s: %p", m.name.c_str(), (void *) (m.baseAddress));
        }
        return -EINVAL;
    } else {
        LOGD("remote dlopen base address of %s is %p", mSharedObjectName.c_str(), (void *) (mPatchBaseAddress));
    }
    return 0;
}

int SysServicePatch::connectToService(int &localSocket, int &remoteSocket) {
    // if we don't have a shared object loaded, cannot connect
    if (mPatchBaseAddress == 0) {
        return -EINVAL;
    }
    if (!mInjector.isAttached()) {
        if (int err = mInjector.attachTargetProcess(); err != 0) {
            return err;
        }
    }
    // create socket connection if not already done
    if (mLocalSocket < 0 || mRemoteSocket < 0) {
        if (int err = mInjector.establishUnixDomainSocket(&mLocalSocket, &mRemoteSocket); err != 0) {
            return err;
        }
    }
    // we need to know the init function address in the target process
    // check the information of the shared object ELF header whether the init function address is known
    struct ElfHeaderLike {
        uint8_t used_bytes[9];
        uint8_t padding1;
        uint8_t padding2;
        uint8_t padding3;
        uint32_t offset;
    };
    static_assert(sizeof(ElfHeaderLike) == 16, "sizeof(ElfHeaderLike) != 16");
    ElfHeaderLike headerLike = {};
    if (int err = mInjector.readRemoteMemory(mPatchBaseAddress, &headerLike, sizeof(headerLike)); err != 0) {
        LOGE("Failed to read ELF header: %d", err);
        return err;
    }
    uint32_t offset = headerLike.offset;
    uintptr_t initFunctionAddress = 0;
    if (uint8_t checksum = (offset & 0xFFu) ^ ((offset >> 8) & 0xFFu)
                           ^ ((offset >> 16) & 0xFFu) ^ ((offset >> 24) & 0xFFu);
            offset != 0 && checksum == headerLike.padding3) {
        // read the info
        halpatch::SharedObjectInfo info = {};
        if (int err = mInjector.readRemoteMemory(mPatchBaseAddress + offset, &info, sizeof(info)); err != 0) {
            return err;
        }
        if (info.magic == halpatch::SharedObjectInfo::SO_INFO_MAGIC) {
            initFunctionAddress = uintptr_t(info.initProcAddress);
        }
    }
    // find init function address from the ELF file if we don't find
    if (initFunctionAddress == 0) {
        if (int err = mInjector.getRemoteDynSymAddress(&initFunctionAddress,
                                                       std::string(mSharedObjectName).c_str(),
                                                       std::string(mInitSymbolName).c_str()); err != 0) {
            return err;
        }
    }
    if (initFunctionAddress == 0) {
        LOGE("Failed to find init function address");
        return -EFAULT;
    }
    // call init function in target process
    uintptr_t retValue = -1;
    if (int err = mInjector.callRemoteProcedure(initFunctionAddress, &retValue,
                                                {uintptr_t(mRemoteSocket)}); err != 0) {
        return err;
    }
    if (retValue != 0) {
        std::string errorMessage = "remote init function reported failure: " + std::to_string(int(retValue));
        LOGE("%s", errorMessage.c_str());
        return -std::abs(int(retValue));
    }
    // return the socket connection, the socket fds will be used by the caller, so we should not touch them anymore
    localSocket = mLocalSocket;
    remoteSocket = mRemoteSocket;
    mLocalSocket = -1;
    mRemoteSocket = -1;
    // usually, this is the last step of the initialization, so we can detach the target process
    if (mInjector.isAttached()) {
        mInjector.detach();
    }
    return 0;
}

int SysServicePatch::getPltHookEntries(OriginHookProcedure &hookProc, std::string_view targetSoname,
                                       uint32_t targetHookEntries) const {
    uintptr_t targetBase = mInjector.getModuleBaseAddress(targetSoname);
    if (targetBase == 0) {
        LOGE("Failed to get base address of %s", targetSoname.data());
        for (const auto &m: mInjector.getProcessView().getModules()) {
            LOGI("%s: %p %s", m.name.c_str(), (void *) (m.baseAddress), m.path.c_str());
        }
        return -ENXIO;
    }
    auto targetSoPath = mInjector.getModulePath(targetSoname);
    if (targetSoPath.empty()) {
        return -ENOENT;
    } else {
        hookProc.struct_size = sizeof(OriginHookProcedure);
        hookProc.entry_count = 9;
        hookProc.target_base = targetBase;
        FileMemMap fileMemMap;
        if (int err = fileMemMap.mapFilePath(targetSoPath.c_str()); err != 0) {
            return -err;
        }
        ElfView elfView;
        elfView.attachFileMemMapping(Rva(fileMemMap.getAddress(), fileMemMap.getLength()));
        {
            auto symname = "__open_2";
            auto addrs = elfView.getExtSymGotRelVirtAddr(symname);
            if (!addrs.empty()) {
                auto addr = addrs[0];
                hookProc.off_plt_open_2 = uint32_t(addr);
            }
        }
        {
            auto symname = "open";
            auto addrs = elfView.getExtSymGotRelVirtAddr(symname);
            if (!addrs.empty()) {
                auto addr = addrs[0];
                hookProc.off_plt_open = uint32_t(addr);
            }
        }
        {
            auto symname = "__write_chk";
            auto addrs = elfView.getExtSymGotRelVirtAddr(symname);
            if (!addrs.empty()) {
                auto addr = addrs[0];
                hookProc.off_plt_write_chk = uint32_t(addr);
            }
        }
        {
            auto symname = "__read_chk";
            auto addrs = elfView.getExtSymGotRelVirtAddr(symname);
            if (!addrs.empty()) {
                auto addr = addrs[0];
                hookProc.off_plt_read_chk = uint32_t(addr);
            }
        }
        {
            auto symname = "ioctl";
            auto addrs = elfView.getExtSymGotRelVirtAddr(symname);
            if (!addrs.empty()) {
                auto addr = addrs[0];
                hookProc.off_plt_ioctl = uint32_t(addr);
            }
        }
        {
            auto symname = "select";
            auto addrs = elfView.getExtSymGotRelVirtAddr(symname);
            if (!addrs.empty()) {
                auto addr = addrs[0];
                hookProc.off_plt_select = uint32_t(addr);
            }
        }
        {
            auto symname = "close";
            auto addrs = elfView.getExtSymGotRelVirtAddr(symname);
            if (!addrs.empty()) {
                auto addr = addrs[0];
                hookProc.off_plt_close = uint32_t(addr);
            }
        }
        if ((((targetHookEntries & PltHookTarget::OPEN) == 0) ||
             (hookProc.off_plt_open != 0 || hookProc.off_plt_open_2 != 0))
            && (((targetHookEntries & PltHookTarget::CLOSE) == 0) || (hookProc.off_plt_close != 0))
            && (((targetHookEntries & PltHookTarget::WRITE) == 0) ||
                (hookProc.off_plt_write_chk != 0 || hookProc.off_plt_write != 0))
            && (((targetHookEntries & PltHookTarget::READ) == 0) ||
                (hookProc.off_plt_read_chk != 0 || hookProc.off_plt_read != 0))
            && (((targetHookEntries & PltHookTarget::IOCTL) == 0) || (hookProc.off_plt_ioctl != 0))) {
            return 0;
        } else {
            LOGE("Failed to get plt hook entries of %s", targetSoname.data());
            LOGE("targetHookEntries: %x", targetHookEntries);
            LOGE("open: %x, open_2: %x, close: %x, write: %x, write_chk: %x, read: %x, read_chk: %x, ioctl: %x",
                 hookProc.off_plt_open, hookProc.off_plt_open_2, hookProc.off_plt_close,
                 hookProc.off_plt_write, hookProc.off_plt_write_chk, hookProc.off_plt_read,
                 hookProc.off_plt_read_chk, hookProc.off_plt_ioctl);
            return -EFAULT;
        }
    }
}
