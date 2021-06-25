//
// Created by kinit on 2021-06-24.
//
#ifndef RPCPROTOCOL_REMOTEDAEMONEXCEPTION_H
#define RPCPROTOCOL_REMOTEDAEMONEXCEPTION_H

#include <exception>
#include <string>
#include <cstdint>

/* do not throw it */
class RemoteDaemonException : public std::exception {
public:
    RemoteDaemonException();

    RemoteDaemonException(uint32_t type, uint32_t status, const char *msg = nullptr);

    RemoteDaemonException(uint32_t type, uint32_t status, const std::string &msg);

    ~RemoteDaemonException() noexcept override;

    const char *what() const noexcept override;

    uint32_t typeId = 0;
    uint32_t statusCode = 0;
    std::string message;
};

#endif //RPCPROTOCOL_REMOTEDAEMONEXCEPTION_H
