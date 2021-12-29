//
// Created by kinit on 2021-10-13.
//

#ifndef NCIHOSTD_NCIHOSTDAEMONIMPL_H
#define NCIHOSTD_NCIHOSTDAEMONIMPL_H

#include <vector>
#include <tuple>
#include <deque>
#include <atomic>

#include "libbasehalpatch/ipc/daemon_ipc_struct.h"
#include "rpcprotocol/protocol/IpcTransactor.h"
#include "rpcprotocol/protocol/BaseIpcObject.h"
#include "rpcprotocol/INciHostDaemon.h"

namespace ipcprotocol {

class NciHostDaemonImpl : public INciHostDaemon, public BaseIpcObject {
public:
    NciHostDaemonImpl() = default;

    ~NciHostDaemonImpl() override = default;

    NciHostDaemonImpl(const NciHostDaemonImpl &) = delete;

    NciHostDaemonImpl &operator=(const NciHostDaemonImpl &) = delete;

    [[nodiscard]] uint32_t getProxyId() const override;

    bool dispatchLpcInvocation(const IpcTransactor::LpcEnv &env, LpcResult &result,
                               uint32_t funcId, const ArgList &args) override;

    bool dispatchEvent(const IpcTransactor::LpcEnv &env, uint32_t eventId, const ArgList &args) override;

    void handleIoSyscallEvent(const halpatch::IoSyscallEvent &event, const std::vector<uint8_t> &payload);

    TypedLpcResult<std::string> getVersionName() override;

    TypedLpcResult<int> getVersionCode() override;

    TypedLpcResult<std::string> getBuildUuid() override;

    TypedLpcResult<int> getSelfPid() override;

    TypedLpcResult<void> exitProcess() override;

    TypedLpcResult<bool> isDeviceSupported() override;

    TypedLpcResult<bool> isHwServiceConnected() override;

    TypedLpcResult<bool> initHwServiceConnection(const std::vector<std::string> &soPath) override;

    TypedLpcResult<HistoryIoOperationEventList> getHistoryIoOperations(uint32_t start, uint32_t length) override;

    TypedLpcResult<bool> clearHistoryIoEvents() override;

    TypedLpcResult<DaemonStatus> getDaemonStatus() override;

    TypedLpcResult<int> deviceDriverWriteRawBuffer(const std::vector<uint8_t> &buffer) override;

    TypedLpcResult<int> deviceDriverIoctl0(uint64_t request, uint64_t arg) override;

    TypedLpcResult<bool> isAndroidNfcServiceConnected() override;

    TypedLpcResult<bool> connectToAndroidNfcService() override;

    TypedLpcResult<bool> isNfcDiscoverySoundDisabled() override;

    TypedLpcResult<bool> setNfcDiscoverySoundDisabled(bool disable) override;

private:
    std::mutex mEventMutex;
    std::deque<std::tuple<halpatch::IoSyscallEvent, std::vector<uint8_t>>> mHistoryIoEvents;
    std::atomic_uint32_t mNextIoEventId = 0;

public:
    inline uint32_t nextIoSyscallEventSequence() {
        return mNextIoEventId++;
    }

};

}

#endif //NCIHOSTD_NCIHOSTDAEMONIMPL_H
