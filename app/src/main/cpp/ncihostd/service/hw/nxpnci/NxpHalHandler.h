//
// Created by kinit on 2021-11-17.
//

#ifndef NCI_HOST_NATIVES_NXPHALHANDLER_H
#define NCI_HOST_NATIVES_NXPHALHANDLER_H

#include <string>
#include <memory>

#include "../../HwServiceStatus.h"
#include "../BaseHwHalHandler.h"

namespace hwhal {

class NxpHalHandler : public BaseHwHalHandler {
public:
    static constexpr const char *EXEC_NAME = "nfc_nci.nqx.default.hw.so";
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

    void dispatchHwHalIoEvent(const halpatch::IoOperationEvent &event, const void *payload) override;

    void dispatchRemoteProcessDeathEvent() override;

    static HwServiceStatus getHwServiceStatus();

private:
    std::string mDescription;
    static std::weak_ptr<IBaseService> sWpInstance;
};

}

#endif //NCI_HOST_NATIVES_NXPHALHANDLER_H
