//
// Created by kinit on 2021-11-17.
//

#ifndef NCI_HOST_NATIVES_NXPHALHANDLER_H
#define NCI_HOST_NATIVES_NXPHALHANDLER_H

#include <string>
#include <memory>
#include <deque>
#include <mutex>
#include <tuple>

#include "rpcprotocol/INciHostDaemon.h"
#include "../../HwServiceStatus.h"
#include "../BaseHwHalHandler.h"

namespace hwhal {

class NxpHalHandler : public BaseHwHalHandler {
public:
    static constexpr const char *EXEC_NAME = "vendor.nxp.hardware.nfc@2.0-service";
    static constexpr const char *TARGET_SO_NAME = "nfc_nci.nqx.default.hw.so";
    static constexpr const char *INIT_SYMBOL = "nxp_hal_patch_inject_init";
    static constexpr const char *DEV_PATH = "/dev/nq-nci";
    static constexpr ssize_t MAX_HISTORY_EVENT_SIZE = 500;

    using StartupInfo = BaseHwHalHandler::StartupInfo;

    NxpHalHandler() = default;

    ~NxpHalHandler() override = default;

    [[nodiscard]] std::string_view getName() const noexcept override;

    [[nodiscard]] std::string_view getDescription() const noexcept override;

    [[nodiscard]] int doOnStart(void *args, const std::shared_ptr<IBaseService> &sp) override;

    [[nodiscard]] bool doOnStop() override;

    [[nodiscard]] int getRemotePltHookStatus();

    [[nodiscard]] int initRemotePltHook(const OriginHookProcedure &hookProc);

    void dispatchHwHalIoEvent(const halpatch::IoOperationEvent &event, const void *payload) override;

    void dispatchRemoteProcessDeathEvent() override;

    [[nodiscard]] ipcprotocol::INciHostDaemon::HistoryIoOperationEventList
    getHistoryIoOperationEvents(uint32_t start, uint32_t count);

    [[nodiscard]] static HwServiceStatus getHwServiceStatus();

    [[nodiscard]] static std::shared_ptr<IBaseService> getWeakInstance();

private:
    std::string mDescription;
    std::mutex mEventMutex;
    std::deque<std::tuple<halpatch::IoOperationEvent, std::vector<uint8_t>>> mHistoryIoEvents;
    static std::weak_ptr<IBaseService> sWpInstance;
};

}

#endif //NCI_HOST_NATIVES_NXPHALHANDLER_H
