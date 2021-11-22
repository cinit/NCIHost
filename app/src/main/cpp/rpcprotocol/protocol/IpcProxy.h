//
// Created by kinit on 2021-10-02.
//

#ifndef RPCPROTOCOL_IPCPROXY_H
#define RPCPROTOCOL_IPCPROXY_H

#include <mutex>
#include <atomic>
#include <unistd.h>

#include "../utils/FixedThreadPool.h"
#include "../utils/ConcurrentHashMap.h"
#include "LpcResult.h"
#include "rpc_struct.h"

namespace ipcprotocol {

class IpcProxy {
public:
    class LpcEnv {
    public:
        const IpcProxy *object;
        const SharedBuffer *buffer;
    };

    class IpcFlags {
    public:
        static constexpr uint32_t IPC_FLAG_CRITICAL_CONTEXT = 0x10;
    };

    using LpcFuncHandler = bool (*)(const LpcEnv &env, LpcResult &result, uint32_t funcId, const ArgList &args);
    using EventHandler = void (*)(const LpcEnv &env, uint32_t funcId, const ArgList &args);
private:
    static constexpr bool sVerboseDebugLog = false;

    class LpcReturnStatus {
    public:
        std::condition_variable *cond;
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
    ConcurrentHashMap<uint32_t, LpcReturnStatus *> mWaitingMap;
    std::atomic<uint32_t> mCurrentSequence = uint32_t((getpid() % 0x4000) << 8);
    LpcFuncHandler mFuncHandler = nullptr;
    EventHandler mEventHandler = nullptr;
public:
    IpcProxy();

    ~IpcProxy();

    IpcProxy(const IpcProxy &) = delete;

    IpcProxy &operator=(const IpcProxy &other) = delete;

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

    inline void setName(const std::string &name) {
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

    int sendRawPacket(const void *buffer, size_t size);

    int sendLpcRequest(uint32_t sequence, uint32_t funId, const SharedBuffer &args);

    int sendLpcResponse(uint32_t sequence, const LpcResult &result);

    int sendLpcResponseError(uint32_t sequence, LpcErrorCode errorId);

    /**
     * Send an event to the end point.
     * Note that this function may block if the event queue is full, this usually happens when the end point is busy
     * or get stuck in D state (Android refrigerator).
     * @param sequence the sequence number of the event
     * @param eventId type of the event
     * @param args event arguments
     * @return 0 on success, errno on failure
     */
    int sendEventSync(uint32_t sequence, uint32_t eventId, const SharedBuffer &args);

    /**
     * Send an event to the end point asynchronously.
     * @param eventId type of the event
     * @param args event arguments
     * @return 0 on success, errno on failure
     */
    int sendEventAsync(uint32_t sequence, uint32_t eventId, const SharedBuffer &args);

    /**
     * Send an event to the end point.
     * @param sync whether to wait for the event to be sent
     * @param eventId event type
     * @param args event arguments
     * @return 0 on success, errno on failure
     */
    int sendEvent(bool sync, uint32_t eventId, const SharedBuffer &args) {
        uint32_t sequence = mCurrentSequence.fetch_add(1, std::memory_order_relaxed);
        if (sync) {
            return sendEventSync(sequence, eventId, args);
        } else {
            return sendEventAsync(sequence, eventId, args);
        }
    }

    [[nodiscard]] LpcResult executeLpcTransaction(uint32_t funcId, uint32_t ipcFlags, const SharedBuffer &args);

    template<class ... Args>
    [[nodiscard]] inline LpcResult executeRemoteProcedure(uint32_t funcId, const Args &...args) {
        return executeLpcTransaction(funcId, 0, ArgList::Builder().pushArgs(args...).build());
    }

    [[nodiscard]] inline EventHandler getEventHandler() const noexcept {
        return mEventHandler;
    }

    [[nodiscard]] inline LpcFuncHandler getFunctionHandler() const noexcept {
        return mFuncHandler;
    }

    void setEventHandler(EventHandler h);

    void setFunctionHandler(LpcFuncHandler h);

private:
    void handleReceivedPacket(const void *buffer, size_t size);

    void dispatchEventPacketAsync(const void *buffer, size_t size);

    void dispatchLpcRequestAsync(const void *buffer, size_t size);

    void handleLpcResponsePacket(const void *buffer, size_t size);

    int detachLocked();

    bool interruptLocked();

    void shutdownLocked();

    void notifyWaitingCalls();

    class LpcFunctionHandleContext {
    public:
        LpcFuncHandler h;
        IpcProxy *object;
        SharedBuffer buffer;
    };

    class EventHandleContext {
    public:
        EventHandler h;
        IpcProxy *object;
        SharedBuffer buffer;
    };

    static void invokeLpcHandler(LpcFunctionHandleContext context);

    static void invokeEventHandler(EventHandleContext context);

    void runIpcLooper();

    static inline void runIpcLooperProc(IpcProxy *that) {
        that->runIpcLooper();
    }
};

}

#endif //RPCPROTOCOL_IPCPROXY_H
