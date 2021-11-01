// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-10-30.
//

#ifndef NCI_HOST_NATIVES_DEFAULTSESSIONLOGIMPL_H
#define NCI_HOST_NATIVES_DEFAULTSESSIONLOGIMPL_H

#include <string>

#include "SessionLog.h"

class DefaultSessionLogImpl : public SessionLog {
private:
    std::string mLogTag;

public:
    explicit DefaultSessionLogImpl(const std::string_view &tag);

    ~DefaultSessionLogImpl() override = default;

    void error(std::string_view msg) override;

    void warn(std::string_view msg) override;

    void info(std::string_view msg) override;

    void verbose(std::string_view msg) override;
};

#endif //NCI_HOST_NATIVES_DEFAULTSESSIONLOGIMPL_H
