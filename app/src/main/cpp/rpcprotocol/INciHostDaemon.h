//
// Created by kinit on 2021-06-09.
//

#ifndef NCI_HOST_INCIHOSTDAEMON_H
#define NCI_HOST_INCIHOSTDAEMON_H

#include <string>

#include "libbasehalpatch/ipc/daemon_ipc_struct.h"
#include "protocol/ArgList.h"
#include "protocol/LpcResult.h"

namespace ipcprotocol {

class INciHostDaemon {
public:
    static constexpr uint32_t PROXY_ID =
            uint32_t('N') | (uint32_t('H') << 8) | (uint32_t('D') << 16) | (uint32_t('0') << 24);

    INciHostDaemon() = default;

    virtual ~INciHostDaemon() = default;

    [[nodiscard]]
    virtual TypedLpcResult<std::string> getVersionName() = 0;

    [[nodiscard]]
    virtual TypedLpcResult<int> getVersionCode() = 0;

    [[nodiscard]]
    virtual TypedLpcResult<std::string> getBuildUuid() = 0;

    virtual TypedLpcResult<void> exitProcess() = 0;

    [[nodiscard]]
    virtual TypedLpcResult<int> testFunction(int value) = 0;

    class TransactionIds {
    public:
        static constexpr uint32_t getVersionName = 1;
        static constexpr uint32_t getVersionCode = 2;
        static constexpr uint32_t getBuildUuid = 3;
        static constexpr uint32_t exitProcess = 4;
        static constexpr uint32_t testFunction = 5;
    };

    class EventIds {
    public:
        static constexpr uint32_t IO_EVENT = 0x10001;
        static constexpr uint32_t REMOTE_DEATH = 0x10002;
    };
};

}

#endif //NCI_HOST_INCIHOSTDAEMON_H
