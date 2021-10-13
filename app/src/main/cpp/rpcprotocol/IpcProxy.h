//
// Created by kinit on 2021-10-02.
//

#ifndef RPCPROTOCOL_IPCPROXY_H
#define RPCPROTOCOL_IPCPROXY_H

#include <mutex>
#include <atomic>
#include <unistd.h>

#include "FixedThreadPool.h"
#include "LpcResult.h"
#include "rpc_struct.h"
#include "ConcurrentHashMap.h"

class IpcProxy {
public:
    class LpcEnv {
    public:
        const IpcProxy *object;
        const SharedBuffer *buffer;
    };

    using LpcFuncHandler = bool (*)(const LpcEnv &env, LpcResult &result, uint32_t funcId, const ArgList &args);
    using EventHandler = void (*)(const LpcEnv &env, uint32_t funcId, const ArgList &args);
private:
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
    std::mutex mTxLock;
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
     * @param fd AF_UNIX DGRAM
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

    void join();

    void start();

    int detach();

    bool interrupt();

    void shutdown();

    int sendRawPacket(const void *buffer, size_t size);

    int sendLpcRequest(uint32_t sequence, uint32_t funId, const SharedBuffer &args);

    int sendLpcResponse(uint32_t sequence, const LpcResult &result);

    int sendLpcResponseError(uint32_t sequence, LpcErrorCode errorId);

    int sendEvent(uint32_t sequence, uint32_t eventId, const SharedBuffer &args);

    [[nodiscard]] LpcResult executeLpcTransaction(uint32_t funcId, const SharedBuffer &args);

    template<class ... Args>
    [[nodiscard]] inline LpcResult executeRemoteProcedure(uint32_t funcId, const Args &...args) {
        return executeLpcTransaction(funcId, ArgList::Builder().pushArgs(args...).build());
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

    class LpcFunctHandleContext {
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

    static void invokeLpcHandler(LpcFunctHandleContext context);

    static void invokeEventHandler(EventHandleContext context);

    void runIpcLooper();

    static inline void runIpcLooperProc(IpcProxy *that) {
        that->runIpcLooper();
    }
};


#endif //RPCPROTOCOL_IPCPROXY_H
