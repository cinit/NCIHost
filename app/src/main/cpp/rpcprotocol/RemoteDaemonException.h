//
// Created by kinit on 2021-06-24.
//
#ifndef RPCPROTOCOL_REMOTEDAEMONEXCEPTION_H
#define RPCPROTOCOL_REMOTEDAEMONEXCEPTION_H

#include <exception>
#include <string>

/* do not throw it */
class RemoteDaemonException : public std::exception {
public:
    RemoteDaemonException();

    RemoteDaemonException(int type, int status, const char *msg = nullptr);

    RemoteDaemonException(int type, int status, const std::string &msg);

    ~RemoteDaemonException() noexcept override;

    const char *what() const noexcept override;

    int typeId = 0;
    int statusCode = 0;
    std::string message;
};

#endif //RPCPROTOCOL_REMOTEDAEMONEXCEPTION_H
