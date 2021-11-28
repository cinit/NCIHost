//
// Created by kinit on 2021-10-02.
//

#ifndef RPCPROTOCOL_IPCPROXY_H
#define RPCPROTOCOL_IPCPROXY_H

#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <string_view>
#include <unistd.h>

#include "../utils/FixedThreadPool.h"
#include "../utils/ConcurrentHashMap.h"
#include "LpcResult.h"
#include "rpc_struct.h"

namespace ipcprotocol {

class BaseIpcObject;

class IpcTransactor {
public:
    class LpcEnv {
    public:
        IpcTransactor *transactor;
        const SharedBuffer *buffer;
    };

    class IpcFlags {
    public:
        static constexpr uint32_t IPC_FLAG_CRITICAL_CONTEXT = 0x10;
    };

private:
    static constexpr bool sVerboseDebugLog = false;

    class LpcReturnStatus {
    public:
        std::condition_variable *cond;
        volatile int semaphore = 0;
        SharedBuffer *buffer;
        volatile uint32_t error;
    };

    int mSocketFd = -1;
    bool mReady = false;
    int mInterruptEventFd = -1;
    FixedThreadPool mExecutor = FixedThreadPool(4);
    std::mutex mStatusLock;
    std::timed_mutex mTxLock;
    std::condition_variable mJoinWaitCondVar;
    std::mutex mRunningEntryMutex; // must be released in finite time
    std::condition_variable mRunningEntryStartCondVar;
    volatile bool mIsRunning = false;
    std::mutex mReadThreadMutex;
    std::string mName;
    std::mutex mWaitingMapMutex; // must be released in finite time
    HashMap<uint32_t, LpcReturnStatus *> mWaitingMap;
    ConcurrentHashMap<uint32_t, BaseIpcObject *> mIpcObjects;
    std::atomic<uint32_t> mCurrentSequence = uint32_t((getpid() % 0x4000) << 8);

public:
    IpcTransactor() = default;

    ~IpcTransactor();

    IpcTransactor(const IpcTransactor &) = delete;

    IpcTransactor &operator=(const IpcTransactor &other) = delete;

    /**
     * reset->attach(connected)->join(running)->interrupt(connected)->runIpcLooper->r:close(disconnected)
     * @param fd AF_UNIX SEQPACKET socket
     * @return 0 success, errno on failure
     */
    [[nodiscard]] int attach(int fd);

    [[nodiscard]] inline bool isConnected() const noexcept {
        return mSocketFd >= 0;
    }

    [[nodiscard]] inline bool isRunning() const noexcept {
        return mIsRunning;
    }

    [[nodiscard]] inline int getFileDescriptor() const noexcept {
        return mSocketFd;
    }

    inline void setName(const char *name) {
        std::scoped_lock<std::mutex> lk(mStatusLock);
        mName = name;
    }

    inline const char *getName() const noexcept {
        return mName.c_str();
    }

    inline void setName(std::string_view name) {
        std::scoped_lock<std::mutex> lk(mStatusLock);
        mName = name;
    }

    /**
     * Wait until the proxy is closed.
     */
    void join();

    void start();

    int detach();

    bool interrupt();

    void shutdown();

    /**
     * Register an IPC object.
     * @param obj the object to register
     * @return true if the object was registered, false if there is already an object with the same id
     */
    bool registerIpcObject(BaseIpcObject &obj);

    /**
     * Unregister an IPC object.
     * @param proxyId the id of the object to unregister
     * @return true if the object was unregistered, false if there is no object with the given id
     */
    bool unregisterIpcObject(uint32_t proxyId);

    /**
     * Test whether an IPC object is registered.
     * @param proxyId the id of the object to test
     * @return true if the object is registered, false if there is no object with the given id
     */
    bool hasIpcObject(uint32_t proxyId) const;

    /**
     * Get all registered IPC objects.
     * @return the registered IPC objects
     */
    std::vector<BaseIpcObject *> getRegisteredIpcObjects() const;

    /**
     * Unregister all registered IPC objects.
     */
    void unregisterAllIpcObjects();

    int sendRawPacket(const void *buffer, size_t size);

    int sendLpcRequest(uint32_t sequence, uint32_t proxyId, uint32_t funId, const SharedBuffer &args);

    int sendLpcResponse(uint32_t sequence, uint32_t proxyId, const LpcResult &result);

    int sendLpcResponseError(uint32_t sequence, uint32_t proxyId, LpcErrorCode errorId);

    /**
     * Send an event to the end point.
     * Note that this function may block if the event queue is full, this usually happens when the end point is busy
     * or get stuck in D state (Android refrigerator).
     * @param sequence the sequence number of the event
     * @param eventId type of the event
     * @param args event arguments
     * @return 0 on success, errno on failure
     */
    int sendEventSync(uint32_t sequence, uint32_t proxyId, uint32_t eventId, const SharedBuffer &args);

    /**
     * Send an event to the end point asynchronously.
     * @param eventId type of the event
     * @param args event arguments
     * @return 0 on success, errno on failure
     */
    int sendEventAsync(uint32_t sequence, uint32_t proxyId, uint32_t eventId, const SharedBuffer &args);

    /**
     * Send an event to the end point.
     * @param sync whether to wait for the event to be sent
     * @param proxyId the proxy id on the end point
     * @param eventId event type
     * @param args event arguments
     * @return 0 on success, errno on failure
     */
    inline int sendEvent(bool sync, uint32_t proxyId, uint32_t eventId, const SharedBuffer &args) {
        uint32_t sequence = mCurrentSequence.fetch_add(1, std::memory_order_relaxed);
        if (sync) {
            return sendEventSync(sequence, proxyId, eventId, args);
        } else {
            return sendEventAsync(sequence, proxyId, eventId, args);
        }
    }

    [[nodiscard]] LpcResult executeLpcTransaction(uint32_t proxyId, uint32_t funcId,
                                                  uint32_t ipcFlags, const SharedBuffer &args);

private:
    void handleReceivedPacket(const void *buffer, size_t size);

    void dispatchEventPacketAsync(uint32_t proxyId, const void *buffer, size_t size);

    void dispatchLpcRequestAsync(uint32_t proxyId, const void *buffer, size_t size);

    void dispatchLpcResponsePacket(uint32_t proxyId, const void *buffer, size_t size);

    int detachLocked();

    bool interruptLocked();

    void shutdownLocked();

    void notifyWaitingCalls();

    class LpcFunctionHandleContext {
    public:
        BaseIpcObject *obj;
        IpcTransactor *transactor;
        SharedBuffer buffer;
    };

    class EventHandleContext {
    public:
        BaseIpcObject *obj;
        IpcTransactor *transactor;
        SharedBuffer buffer;
    };

    static void dispatchFunctionCallToIpcObject(LpcFunctionHandleContext context);

    static void dispatchEventToIpcObject(EventHandleContext context);

    void runIpcLooper();

    static inline void runIpcLooperProc(IpcTransactor *that) {
        that->runIpcLooper();
    }
};

}

#endif //RPCPROTOCOL_IPCPROXY_H
