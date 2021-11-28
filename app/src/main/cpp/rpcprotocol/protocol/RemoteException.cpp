//
// Created by kinit on 2021-06-24.
//

#include "RemoteException.h"
#include <sstream>

using namespace ipcprotocol;

RemoteException::RemoteException() = default;

RemoteException::~RemoteException() noexcept = default;

RemoteException::RemoteException(uint32_t type, uint32_t err, const char *msg)
        : typeId(type), errorCode(err) {
    std::ostringstream s;
    s << type << ":" << err;
    if (msg != nullptr) {
        message = msg;
        s << " " << msg;
    }
    showMessage = s.str();
}

RemoteException::RemoteException(uint32_t type, uint32_t err, const std::string &msg)
        : typeId(type), errorCode(err) {
    std::ostringstream s;
    s << type << ":" << err;
    if (!msg.empty()) {
        message = msg;
        s << " " << msg;
    }
    showMessage = s.str();
}

const char *RemoteException::what() const noexcept {
    return showMessage.c_str();
}
