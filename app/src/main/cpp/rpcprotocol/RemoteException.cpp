//
// Created by kinit on 2021-06-24.
//

#include "RemoteException.h"
#include <sstream>

RemoteException::RemoteException() = default;

RemoteException::~RemoteException() noexcept = default;

RemoteException::RemoteException(uint32_t type, uint32_t status, const char *msg)
        : typeId(type), statusCode(status) {
    std::ostringstream s;
    s << type << ":" << status;
    if (msg != nullptr) {
        message = msg;
        s << " " << msg;
    }
    showMessage = s.str();
}

RemoteException::RemoteException(uint32_t type, uint32_t status, const std::string &msg)
        : typeId(type), statusCode(status) {
    std::ostringstream s;
    s << type << ":" << status;
    if (!msg.empty()) {
        message = msg;
        s << " " << msg;
    }
    showMessage = s.str();
}

const char *RemoteException::what() const noexcept {
    return showMessage.c_str();
}
