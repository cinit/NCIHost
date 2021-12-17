//
// Created by kinit on 2021-06-09.
//

#ifndef NCI_HOST_INCIHOSTDAEMON_H
#define NCI_HOST_INCIHOSTDAEMON_H

#include <string>
#include <vector>

#include "../libbasehalpatch/ipc/daemon_ipc_struct.h"
#include "protocol/ArgList.h"
#include "protocol/LpcResult.h"

namespace ipcprotocol {

class INciHostDaemon {
public:
    static constexpr uint32_t PROXY_ID =
            uint32_t('N') | (uint32_t('H') << 8) | (uint32_t('D') << 16) | (uint32_t('0') << 24);

    class HistoryIoOperationEventList {
    public:
        uint32_t totalStartIndex = 0;
        uint32_t totalCount = 0;
        std::vector<halpatch::IoOperationEvent> events; // contains index and length
        std::vector<std::vector<uint8_t>> payloads;

        [[nodiscard]] bool deserializeFromByteVector(const std::vector<uint8_t> &src);

        [[nodiscard]] std::vector<uint8_t> serializeToByteVector() const;
    };

    class DaemonStatus {
    public:
        int processId;
        std::string versionName;
        int abiArch;
        std::string daemonProcessSecurityContext;
        bool isHalServiceAttached;
        int halServicePid;
        int halServiceUid;
        std::string halServiceExePath;
        int halServiceArch;
        std::string halServiceProcessSecurityContext;
        std::string halServiceExecutableSecurityLabel;

        [[nodiscard]] bool deserializeFromByteVector(const std::vector<uint8_t> &src);

        [[nodiscard]] std::vector<uint8_t> serializeToByteVector() const;
    };

    INciHostDaemon() = default;

    virtual ~INciHostDaemon() = default;

    [[nodiscard]]
    virtual TypedLpcResult<std::string> getVersionName() = 0;

    [[nodiscard]]
    virtual TypedLpcResult<int> getVersionCode() = 0;

    [[nodiscard]]
    virtual TypedLpcResult<std::string> getBuildUuid() = 0;

    [[nodiscard]]
    virtual TypedLpcResult<int> getSelfPid() = 0;

    virtual TypedLpcResult<void> exitProcess() = 0;

    [[nodiscard]]
    virtual TypedLpcResult<bool> isDeviceSupported() = 0;

    [[nodiscard]]
    virtual TypedLpcResult<bool> isHwServiceConnected() = 0;

    [[nodiscard]]
    virtual TypedLpcResult<bool> initHwServiceConnection(const std::string &soPath) = 0;

    [[nodiscard]]
    virtual TypedLpcResult<HistoryIoOperationEventList> getHistoryIoOperations(uint32_t start, uint32_t length) = 0;

    [[nodiscard]]
    virtual TypedLpcResult<bool> clearHistoryIoEvents() = 0;

    [[nodiscard]]
    virtual TypedLpcResult<DaemonStatus> getDaemonStatus() = 0;

    [[nodiscard]]
    virtual TypedLpcResult<int> deviceDriverWriteRawBuffer(const std::vector<uint8_t> &buffer) = 0;

    [[nodiscard]]
    virtual TypedLpcResult<int> deviceDriverIoctl0(uint64_t request, uint64_t arg) = 0;

    class TransactionIds {
    public:
        static constexpr uint32_t getVersionName = 1;
        static constexpr uint32_t getVersionCode = 2;
        static constexpr uint32_t getBuildUuid = 3;
        static constexpr uint32_t exitProcess = 4;
        static constexpr uint32_t isDeviceSupported = 10;
        static constexpr uint32_t isHwServiceConnected = 11;
        static constexpr uint32_t initHwServiceConnection = 12;
        static constexpr uint32_t getSelfPid = 13;
        static constexpr uint32_t getHistoryIoOperations = 14;
        static constexpr uint32_t clearHistoryIoEvents = 15;
        static constexpr uint32_t getDaemonStatus = 16;
        static constexpr uint32_t deviceDriverWriteRawBuffer = 17;
        static constexpr uint32_t deviceDriverIoctl0 = 18;
    };
};

}

#endif //NCI_HOST_INCIHOSTDAEMON_H
