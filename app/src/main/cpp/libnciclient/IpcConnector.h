//
// Created by kinit on 2021-10-11.
//

#ifndef NCICLIENT_IPCCONNECTOR_H
#define NCICLIENT_IPCCONNECTOR_H

#include <mutex>

#include "../rpcprotocol/IpcProxy.h"

class IpcConnector {
public:
    enum class IpcStatusEvent {
        IPC_CONNECTED = 1,
        IPC_DISCONNECTED = 2,
    };

    using IpcStatusListener = void (*)(IpcStatusEvent event, IpcProxy *obj);

    ~IpcConnector();

    IpcConnector(const IpcConnector &) = delete;

    IpcConnector &operator=(const IpcConnector &other) = delete;

    bool isConnected();

    void registerIpcStatusListener(IpcStatusListener *listener);

    bool unregisterIpcStatusListener(IpcStatusListener *listener);

    /**
     * return null if not connected
     */
    IpcProxy *getIpcProxy();

    void sendConnectRequest();

    static IpcConnector &getInstance();

private:
    static IpcConnector *volatile sInstance;
    std::mutex mLock;
    std::shared_ptr<IpcProxy> mIpcProxy;

    explicit IpcConnector();

    void init();
};


#endif //NCICLIENT_IPCCONNECTOR_H
