//
// Created by kinit on 2021-11-17.
//

#ifndef NCI_HOST_NATIVES_NXPHALHANDLER_H
#define NCI_HOST_NATIVES_NXPHALHANDLER_H

#include <string>

#include "../BaseHwHalHandler.h"

namespace hwhal {

class NxpHalHandler : public BaseHwHalHandler {
public:
    using StartupInfo = BaseHwHalHandler::StartupInfo;

    NxpHalHandler() = default;

    ~NxpHalHandler() override = default;

    [[nodiscard]] std::string_view getName() const noexcept override;

    [[nodiscard]] std::string_view getDescription() const noexcept override;

    [[nodiscard]] int doOnStart(void *args) override;

    [[nodiscard]] bool doOnStop() override;

    [[nodiscard]] int getRemotePltHookStatus();

    [[nodiscard]] int initRemotePltHook(const OriginHookProcedure &hookProc);

    void dispatchHwHalIoEvent(const IoOperationEvent &event, const void *payload) override;

    void dispatchRemoteProcessDeathEvent() override;

private:
    std::string mDescription;
};

}

#endif //NCI_HOST_NATIVES_NXPHALHANDLER_H
