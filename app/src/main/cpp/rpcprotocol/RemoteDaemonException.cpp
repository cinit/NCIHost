//
// Created by kinit on 2021-06-24.
//

#include "RemoteDaemonException.h"
#include <sstream>

RemoteDaemonException::RemoteDaemonException() = default;

RemoteDaemonException::~RemoteDaemonException() noexcept = default;

RemoteDaemonException::RemoteDaemonException(int type, int status, const char *msg)
        : typeId(type), statusCode(status) {
    std::ostringstream s;
    s << type << ":" << status;
    if (msg != nullptr) {
        s << " " << msg;
    }
    message = s.str();
}

RemoteDaemonException::RemoteDaemonException(int type, int status, const std::string &msg)
        : typeId(type), statusCode(status) {
    std::ostringstream s;
    s << type << ":" << status;
    if (!msg.empty()) {
        s << " " << msg;
    }
    message = s.str();
}

const char *RemoteDaemonException::what() const noexcept {
    return message.c_str();
}
