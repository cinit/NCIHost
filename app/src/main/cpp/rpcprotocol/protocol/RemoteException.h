//
// Created by kinit on 2021-06-24.
//
#ifndef RPCPROTOCOL_REMOTEEXCEPTION_H
#define RPCPROTOCOL_REMOTEEXCEPTION_H

#include <exception>
#include <string>
#include <cstdint>

namespace ipcprotocol {

/* do not throw it */
class RemoteException : public std::exception {
public:
    RemoteException();

    RemoteException(uint32_t type, uint32_t err, const char *msg = nullptr);

    RemoteException(uint32_t type, uint32_t err, const std::string &msg);

    ~RemoteException() noexcept override;

    const char *what() const noexcept override;

    uint32_t typeId = 0;
    uint32_t errorCode = 0;
    std::string message;
private:
    std::string showMessage;
};

}

#endif //RPCPROTOCOL_REMOTEEXCEPTION_H
