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
    static constexpr const char *EXEC_NAME_BASE = "vendor.nxp.hardware.nfc@";
    static constexpr const char *EXEC_NAME_SUFFIX = "-service";
    static constexpr const char *TARGET_SO_NAME = "nfc_nci.nqx.default.hw.so";
    static constexpr const char *INIT_SYMBOL = "nxp_hal_patch_inject_init";
    static constexpr const char *DEV_PATH = "/dev/nq-nci";

    using StartupInfo = BaseHwHalHandler::StartupInfo;

    NxpHalHandler() = default;

    ~NxpHalHandler() override = default;

    [[nodiscard]] std::string_view getName() const noexcept override;

    [[nodiscard]] std::string_view getDescription() const noexcept override;

    [[nodiscard]] int doOnStart(void *args, const std::shared_ptr<IBaseService> &sp) override;

    [[nodiscard]] bool doOnStop() override;

    [[nodiscard]] int getRemotePltHookStatus();

    [[nodiscard]] int initRemotePltHook(const OriginHookProcedure &hookProc);

    void dispatchHwHalIoEvent(const halpatch::IoSyscallEvent &event, const void *payload) override;

    void dispatchRemoteProcessDeathEvent() override;

    [[nodiscard]] int driverRawWrite(const std::vector<uint8_t> &buffer) override;

    [[nodiscard]] int driverRawIoctl0(uint32_t request, uint64_t arg) override;

    [[nodiscard]] static HwServiceStatus getHwServiceStatus();

    [[nodiscard]] static std::shared_ptr<IBaseService> getWeakInstance();

private:
    std::string mDescription;
    static std::weak_ptr<IBaseService> sWpInstance;
};

}

#endif //NCI_HOST_NATIVES_NXPHALHANDLER_H
