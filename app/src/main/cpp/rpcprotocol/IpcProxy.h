//
// Created by kinit on 2021-10-02.
//

#ifndef RPCPROTOCOL_IPCPROXY_H
#define RPCPROTOCOL_IPCPROXY_H

#include "ThreadPool.h"

class IpcProxy {
private:
    int mSocketFd = -1;
    ThreadPool mExecutor;

public:
    IpcProxy();

    IpcProxy(const IpcProxy &) = delete;

    IpcProxy &operator=(const IpcProxy &other) = delete;

    [[nodiscard]] int attach(int fd);

    [[nodiscard]] bool isConnected();

    [[nodiscard]] int getFileDescriptor();

    int detach();

    void close();

    int executeRemoteProcedure();
};


#endif //RPCPROTOCOL_IPCPROXY_H
