//
// Created by kinit on 2021-10-11.
//

#ifndef NCICLIENT_IPCCONNECTOR_H
#define NCICLIENT_IPCCONNECTOR_H

#include <mutex>
#include <atomic>

#include "../rpcprotocol/IpcProxy.h"
#include "NciHostDaemonProxy.h"

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

    [[nodiscard]] bool isConnected();

    void registerIpcStatusListener(IpcStatusListener listener);

    bool unregisterIpcStatusListener(IpcStatusListener listener);

    /**
     * return null if not connected
     */
    [[nodiscard]]
    IpcProxy *getIpcProxy();

    [[nodiscard]]
    INciHostDaemon *getNciDaemon();

    int sendConnectRequest();

    [[nodiscard]] int waitForConnection(int timeout_ms);

    [[nodiscard]] inline int getIpcFileFlagFd() const noexcept {
        return mIpcFileFlagFd;
    }

    /**
     * @param dirPath "/data/data/pkg/files"
     * @return 0 for success, errno for errors
     */
    [[nodiscard]] int init(const char *dirPath, const char *uuidStr);

    [[nodiscard]] inline bool isInitialized() const noexcept {
        return mInitialized;
    }

    [[nodiscard]] static IpcConnector &getInstance();

private:
    static IpcConnector *volatile sInstance;
    pthread_t mIpcListenThread = 0;
    bool mInitialized = false;
    int mIpcListenFd = -1;
    int mIpcConnFd = -1;
    int mIpcFileFlagFd = -1;
    std::mutex mLock;
    std::string mIpcAbsSocketName;
    std::string mIpcPidFilePath;
    std::condition_variable mConnCondVar;
    std::shared_ptr<IpcProxy> mIpcProxy;
    std::shared_ptr<NciHostDaemonProxy> mNciProxy;
    std::vector<IpcStatusListener> mStatusListeners;

    explicit IpcConnector();

    void listenForConnInternal();

    void handleLinkStart(int fd);

    bool handleSocketReconnect(int client);

    static inline void stpListenForConnInternal(IpcConnector *that) {
        that->listenForConnInternal();
    }
};


#endif //NCICLIENT_IPCCONNECTOR_H
