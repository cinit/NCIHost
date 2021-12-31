//
// Created by kinit on 2021-10-02.
//

#include <unistd.h>
#include <cerrno>
#include <chrono>
#include <sstream>
#include <sys/fcntl.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "rpc_struct.h"
#include "IpcTransactor.h"
#include "BaseIpcObject.h"
#include "../log/Log.h"

#define LOG_TAG "IpcProxy"

using namespace ipcprotocol;

IpcTransactor::~IpcTransactor() {
    shutdown();
}

int IpcTransactor::attach(int fd) {
    std::scoped_lock<std::mutex> lock(mStatusLock);
    if (fd < 0) {
        return EBADFD;
    }
    // check socket type
    int sock_type;
    if (socklen_t optlen = 4; getsockopt(fd, SOL_SOCKET, SO_TYPE, (void *) &sock_type, &optlen) < 0) {
        int err = errno;
        LOGE("get socket type failed: %s", strerror(err));
        return err;
    }
    if (sock_type != SOCK_SEQPACKET) {
        LOGE("socket type error, expect SOCK_SEQPACKET, got %d", sock_type);
        return EINVAL;
    }
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        return errno;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return errno;
    }
    mSocketFd = fd;
    return 0;
}

void IpcTransactor::join() {
    if (sVerboseDebugLog) {
        LOGV("---> join() start");
    }
    {
        std::unique_lock lk(mStatusLock);
        if (mIsRunning) {
            mJoinWaitCondVar.wait(lk);
        }
    }
    if (sVerboseDebugLog) {
        LOGV("---> join() end");
    }
}

void IpcTransactor::start() {
    {
        std::scoped_lock<std::mutex> lk(mStatusLock);
        std::unique_lock startLock(mRunningEntryMutex);
        if (!mReady) {
            if (mInterruptEventFd == -1) {
                mInterruptEventFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
            }
            if (mInterruptEventFd == -1) {
                std::string errmsg;
                std::stringstream ss;
                ss << "create eventfd error:" << errno;
                ss >> errmsg;
                LOGE("%s create eventfd error %d,%s %s", mName.c_str(), errno, strerror(errno), mName.c_str());
                throw std::runtime_error(errmsg);
            }
            mExecutor.start();
            mReady = true;
        }
        pthread_t tid = 0;
        int result = pthread_create(&tid, nullptr,
                                    reinterpret_cast<void *(*)(void *)>(&IpcTransactor::runIpcLooperProc), this);
        if (result != 0) {
            char buf[64];
            snprintf(buf, 64, "create thread failed errno=%d", result);
            LOGE("create thread failed errno=%d", result);
            throw std::runtime_error(buf);
        }
        mRunningEntryStartCondVar.wait(startLock);
    }
}

int IpcTransactor::detach() {
    std::scoped_lock<std::mutex> lk(mStatusLock);
    return detachLocked();
}

bool IpcTransactor::interrupt() {
    std::scoped_lock<std::mutex> lk(mStatusLock);
    return interruptLocked();
}

void IpcTransactor::shutdown() {
    std::scoped_lock<std::mutex> lk(mStatusLock);
    shutdownLocked();
}

int IpcTransactor::detachLocked() {
    if (mSocketFd == -1) {
        return -1;
    }
    if (mIsRunning) {
        interruptLocked();
    }
    int fd = mSocketFd;
    mSocketFd = -1;
    notifyWaitingCalls();
    return fd;
}

bool IpcTransactor::interruptLocked() {
    if (mIsRunning) {
        uint64_t u64 = 1;
        write(mInterruptEventFd, &u64, 8);
        // TODO: remove this potential dead lock
        std::scoped_lock<std::mutex> wait_for_exit(mReadThreadMutex);
        return true;
    } else {
        return false;
    }
}

void IpcTransactor::shutdownLocked() {
    mExecutor.shutdown();
    if (mSocketFd >= 0) {
        int fd = detachLocked();
        close(fd);
    }
    if (mInterruptEventFd != -1) {
        close(mInterruptEventFd);
        mInterruptEventFd = -1;
    }
    mReady = false;
}

void IpcTransactor::notifyWaitingCalls() {
    std::scoped_lock<std::mutex> lk(mWaitingMapMutex);
    for (auto &it: mWaitingMap.entrySet()) {
        if (LpcReturnStatus **pStatus = it->getValue();
                pStatus != nullptr) {
            LpcReturnStatus *status = *pStatus;
            status->buffer->reset();
            status->error = static_cast<uint32_t>(LpcErrorCode::ERR_BROKEN_CONN);
            status->cond->notify_all();
        }
        mWaitingMap.remove(it->getKey());
    }
}

void IpcTransactor::runIpcLooper() {
    // do NOT touch mStatusMutex here
    SharedBuffer sharedBuffer;
    sharedBuffer.ensureCapacity(65536);
    void *buffer = sharedBuffer.get();
    int epollFd = -1;
    std::scoped_lock<std::mutex> runLock(mReadThreadMutex);
    {
        std::scoped_lock<std::mutex> lk(mRunningEntryMutex);
        pthread_setname_np(pthread_self(), "IPC-trxn");
        if (mSocketFd >= 0 && mInterruptEventFd >= 0) {
            epollFd = epoll_create(2);
            epoll_event ev = {};
            ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
            ev.data.fd = mSocketFd;
            if (epoll_ctl(epollFd, EPOLL_CTL_ADD, mSocketFd, &ev) < 0) {
                LOGE("epoll_ctl failed, %d, %s %s", errno, strerror(errno), mName.c_str());
                close(epollFd);
                return;
            }
            ev.events = EPOLLIN;
            ev.data.fd = mInterruptEventFd;
            if (epoll_ctl(epollFd, EPOLL_CTL_ADD, mInterruptEventFd, &ev) < 0) {
                LOGE("epoll_ctl failed, %d, %s %s", errno, strerror(errno), mName.c_str());
                close(epollFd);
                return;
            }
            mIsRunning = true;
        }
        // set all status before unlock!
        mRunningEntryStartCondVar.notify_all();
    }
    if (sVerboseDebugLog) {
        LOGD("pid=%d, mSocketFd=%d, mInterruptEventFd=%d %s", getpid(), mSocketFd, mInterruptEventFd, mName.c_str());
    }
    while (epollFd != -1 && mSocketFd != -1 && mInterruptEventFd != -1) {
        epoll_event event = {};
        if (epoll_wait(epollFd, &event, 1, -1) < 0) {
            int err = errno;
            if (err != EINTR) {
                LOGE("epoll_wait error: %d %s", err, mName.c_str());
                abort();
                break;
            }
        } else {
            if (sVerboseDebugLog) {
                LOGD("epoll_wait return ev=%d fd=%d %s", event.events, event.data.fd, mName.c_str());
            }
            if ((event.events & (EPOLLERR | EPOLLHUP)) != 0) {
                LOGI("pipe close, exit... %s", mName.c_str());
                close(mSocketFd);
                notifyWaitingCalls();
                mSocketFd = -1;
                break;
            } else if (event.data.fd == mInterruptEventFd) {
                uint64_t u64 = 0;
                // consume event
                read(mInterruptEventFd, &u64, 8);
                LOGI("interrupt event received, exit... %s", mName.c_str());
                break;
            } else if (event.data.fd == mSocketFd) {
                if (sVerboseDebugLog) {
                    LOGV("epoll: socket fd, %d %s", event.events, mName.c_str());
                }
                int size = (int) recv(mSocketFd, buffer, 65536, 0);
                if (size < 0) {
                    int err = errno;
                    if (err != EINTR) {
                        if (err == EPIPE) {
                            LOGI("pipe close, exit... %s", mName.c_str());
                            close(mSocketFd);
                            notifyWaitingCalls();
                            mSocketFd = -1;
                            break;
                        } else {
                            LOGE("recv() failed, %d, %s %s", errno, strerror(errno), mName.c_str());
                            close(mSocketFd);
                            notifyWaitingCalls();
                            mSocketFd = -1;
                            break;
                        }
                    }
                } else {
                    uint32_t trunkSize = *(uint32_t *) (buffer);
                    if (trunkSize != size) {
                        LOGE("read size(%u) does not match chunk size(%u) %s", size, trunkSize, mName.c_str());
                    } else {
                        handleReceivedPacket(buffer, size);
                    }
                }
            } else if (event.events == 0) {
                LOGI("EAGAIN %s", mName.c_str());
                continue;
            } else {
                LOGE("unexpected event");
                abort();
            }
        }
    }
    mIsRunning = false;
    mJoinWaitCondVar.notify_all();
    if (epollFd != -1) {
        close(epollFd);
    }
}

int IpcTransactor::sendRawPacket(const void *buffer, size_t size) {
    std::scoped_lock lk(mTxLock);
    if (sVerboseDebugLog) {
        LOGD("send size %zu %s", size, mName.c_str());
    }
    ssize_t result = send(mSocketFd, buffer, size, 0);
    if (result != size) {
        if (result < 0) {
            LOGW("send packet error %d %s %s", errno, strerror(errno), mName.c_str());
            return errno;
        } else {
            LOGW("send %zud but only %zd %s", size, result, mName.c_str());
            return 0;
        }
    } else {
        return 0;
    }
}

int IpcTransactor::sendLpcRequest(uint32_t sequence, uint32_t proxyId, uint32_t funId, const SharedBuffer &args) {
    SharedBuffer result;
    result.ensureCapacity(sizeof(LpcTransactionHeader) + args.size());
    memcpy(result.at<char>(sizeof(LpcTransactionHeader)), args.get(), args.size());
    auto *h = result.at<LpcTransactionHeader>(0);
    h->header.length = (uint32_t) result.size();
    h->header.type = TrxnType::TRXN_TYPE_RPC;
    h->header.proxyId = proxyId;
    h->header.rfu4_0 = 0;
    h->funcId = funId;
    h->sequence = sequence;
    h->rpcFlags = 0;
    h->errorCode = 0;
    return sendRawPacket(result.get(), result.size());
}

int IpcTransactor::sendLpcResponse(uint32_t sequence, uint32_t proxyId, const LpcResult &result) {
    SharedBuffer buf = result.buildLpcResponsePacket(sequence, proxyId);
    if (buf.get() != nullptr) {
        return sendRawPacket(buf.get(), buf.size());
    } else {
        return EINVAL;
    }
}

int IpcTransactor::sendLpcResponseError(uint32_t sequence, uint32_t proxyId, LpcErrorCode errorId) {
    SharedBuffer buf;
    buf.ensureCapacity(sizeof(LpcTransactionHeader));
    auto *h = buf.at<LpcTransactionHeader>(0);
    h->header.length = sizeof(LpcTransactionHeader);
    h->header.type = TrxnType::TRXN_TYPE_RPC;
    h->header.proxyId = proxyId;
    h->header.rfu4_0 = 0;
    h->errorCode = static_cast<uint32_t>(errorId);
    h->sequence = sequence;
    h->rpcFlags = LPC_FLAG_ERROR;
    h->funcId = 0;
    return sendRawPacket(buf.get(), buf.size());
}

int IpcTransactor::sendEventSync(uint32_t sequence, uint32_t proxyId, uint32_t eventId, const SharedBuffer &args) {
    SharedBuffer result;
    if (!result.ensureCapacity(sizeof(EventTransactionHeader) + args.size(), std::nothrow_t())) {
        throw std::bad_alloc();
    }
    memcpy(result.at<char>(sizeof(EventTransactionHeader)), args.get(), args.size());
    auto *h = result.at<EventTransactionHeader>(0);
    h->header.length = (uint32_t) result.size();
    h->header.type = TrxnType::TRXN_TYPE_EVENT;
    h->header.proxyId = proxyId;
    h->header.rfu4_0 = 0;
    h->eventId = eventId;
    h->sequence = sequence;
    h->eventFlags = 0;
    h->reserved = 0;
    return sendRawPacket(result.get(), result.size());
}

int IpcTransactor::sendEventAsync(uint32_t sequence, uint32_t proxyId, uint32_t eventId, const SharedBuffer &args) {
    if (mSocketFd == -1) {
        // no socket, no async events
        return EBADFD;
    }
    SharedBuffer result;
    if (!result.ensureCapacity(sizeof(EventTransactionHeader) + args.size(), std::nothrow_t())) {
        throw std::bad_alloc();
    }
    memcpy(result.at<char>(sizeof(EventTransactionHeader)), args.get(), args.size());
    auto *h = result.at<EventTransactionHeader>(0);
    h->header.length = (uint32_t) result.size();
    h->header.type = TrxnType::TRXN_TYPE_EVENT;
    h->header.proxyId = proxyId;
    h->header.rfu4_0 = 0;
    h->eventId = eventId;
    h->sequence = sequence;
    h->eventFlags = 0;
    h->reserved = 0;
    if (mTxLock.try_lock_for(std::chrono::milliseconds(20))) {
        // set the O_NONBLOCK flag
        fcntl(mSocketFd, F_SETFL, O_NONBLOCK);
        ssize_t rc = send(mSocketFd, result.get(), result.size(), 0);
        // clear the O_NONBLOCK flag and unlock the mutex
        fcntl(mSocketFd, F_SETFL, fcntl(mSocketFd, F_GETFL) & ~O_NONBLOCK);
        mTxLock.unlock();
        if (rc < 0) {
            int err = errno;
            LOGW("send event async error %d %s %s", errno, strerror(errno), mName.c_str());
            return err;
        } else {
            return 0;
        }
    } else {
        LOGW("send event async failed to acquire lock %s", mName.c_str());
        return EBUSY;
    }
}

LpcResult IpcTransactor::executeLpcTransaction(uint32_t proxyId, uint32_t funcId,
                                               uint32_t ipcFlags, const SharedBuffer &args) {
    LpcResult result;
    uint32_t seq = mCurrentSequence.fetch_add(1);
    SharedBuffer buffer;
    std::condition_variable condvar;
    LpcReturnStatus returnStatus = {&condvar, 0, &buffer, 0};
    {
        std::scoped_lock<std::mutex> lk(mRunningEntryMutex);
        if (!isConnected()) {
            result.setError(LpcErrorCode::ERR_BROKEN_CONN);
            return result;
        }
        if (!mIsRunning) {
            result.setError(LpcErrorCode::ERR_LOCAL_INTERNAL_ERROR);
            return result;
        }
        std::unique_lock retlk(mWaitingMapMutex);
        mWaitingMap.put(seq, &returnStatus);
    }
    if (sendLpcRequest(seq, proxyId, funcId, args) != 0) {
        std::unique_lock retlk(mWaitingMapMutex);
        mWaitingMap.remove(seq);
        result.setError(LpcErrorCode::ERR_LOCAL_INTERNAL_ERROR);
        return result;
    }
    // request sent, wait for response, note that a response may arrive before we wait
    std::unique_lock retlk(mWaitingMapMutex);
    if (returnStatus.semaphore == 0) {
        // response not arrived, wait for it
        if ((ipcFlags & IpcFlags::IPC_FLAG_CRITICAL_CONTEXT) != 0) {
            if (condvar.wait_for(retlk, std::chrono::milliseconds(1000)) == std::cv_status::timeout) {
                mWaitingMap.remove(seq);
                result.setError(LpcErrorCode::ERR_TIMEOUT_IN_CRITICAL_CONTEXT);
                return result;
            }
        } else {
            condvar.wait(retlk);
        }
    }
    mWaitingMap.remove(seq);
    if (buffer.get() != nullptr) {
        return LpcResult::fromLpcRespPacketBuffer(buffer);
    } else {
        result.setError((LpcErrorCode) returnStatus.error);
    }
    return result;
}

void IpcTransactor::handleReceivedPacket(const void *buffer, size_t size) {
    if (size < 24) {
        LOGE("packet too small(%zu), ignore  %s", size, mName.c_str());
        return;
    }
    if (size > 64 * 1024 * 1024) {
        LOGE("packet too large(%zu), ignore %s", size, mName.c_str());
        return;
    }
    const auto *header = reinterpret_cast<const TrxnPacketHeader *>(buffer);
    auto type = header->type;
    uint32_t proxyId = header->proxyId;
    if (type == TrxnType::TRXN_TYPE_RPC) {
        if (reinterpret_cast<const LpcTransactionHeader *>(buffer)->funcId == 0) {
            dispatchLpcResponsePacket(proxyId, buffer, size);
        } else {
            dispatchLpcRequestAsync(proxyId, buffer, size);
        }
    } else if (type == TrxnType::TRXN_TYPE_EVENT) {
        dispatchEventPacketAsync(proxyId, buffer, size);
    } else {
        LOGE("unknown type (%ud), ignore %s", type, mName.c_str());
    }
}

void IpcTransactor::dispatchEventPacketAsync(uint32_t proxyId, const void *buffer, size_t size) {
    uint32_t seq = reinterpret_cast<const LpcTransactionHeader *>(buffer)->sequence;
    auto **ipcObj = mIpcObjects.get(proxyId);
    if (ipcObj != nullptr) {
        SharedBuffer sb;
        if (!sb.ensureCapacity(size, std::nothrow_t())) {
            LOGE("failed to allocate buffer size = %zu, drop event  %s", size, mName.c_str());
            return;
        }
        memcpy(sb.get(), buffer, size);
        EventHandleContext context = {*ipcObj, this, sb};
        mExecutor.execute([context]() {
            IpcTransactor::dispatchEventToIpcObject(context);
        });
    } else {
        LOGW("event received for unknown proxy id %u %s", proxyId, mName.c_str());
    }
}

void IpcTransactor::dispatchLpcRequestAsync(uint32_t proxyId, const void *buffer, size_t size) {
    uint32_t seq = reinterpret_cast<const LpcTransactionHeader *>(buffer)->sequence;
    auto **ipcObj = mIpcObjects.get(proxyId);
    if (ipcObj != nullptr) {
        SharedBuffer sb;
        if (!sb.ensureCapacity(size, std::nothrow_t())) {
            sendLpcResponseError(seq, proxyId, LpcErrorCode::ERR_REMOTE_INTERNAL_ERROR);
            LOGE("failed to allocate buffer size = %zu, return lpc error  %s", size, mName.c_str());
            return;
        }
        memcpy(sb.get(), buffer, size);
        LpcFunctionHandleContext context = {*ipcObj, this, sb};
        mExecutor.execute([context]() {
            IpcTransactor::dispatchFunctionCallToIpcObject(context);
        });
    } else {
        sendLpcResponseError(seq, proxyId, LpcErrorCode::ERR_NO_REMOTE_OBJECT);
    }
}

void IpcTransactor::dispatchLpcResponsePacket(uint32_t, const void *buffer, size_t size) {
    uint32_t seq = reinterpret_cast<const LpcTransactionHeader *>(buffer)->sequence;
    LpcReturnStatus **pStatus;
    {
        std::scoped_lock lk(mWaitingMapMutex);
        pStatus = mWaitingMap.get(seq);
    }
    if (pStatus != nullptr) {
        std::scoped_lock lk(mWaitingMapMutex);
        LpcReturnStatus *status = *pStatus;
        status->semaphore = 1;
        if (status->buffer->resetSize(size, false)) {
            memcpy(status->buffer->get(), buffer, size);
            status->error = 0;
        } else {
            status->error = static_cast<uint32_t>(LpcErrorCode::ERR_LOCAL_INTERNAL_ERROR);
        }
        status->cond->notify_all();
    } else {
        LOGE("dispatchLpcResponsePacket but no LpcReturnStatus found, seq=%ud  %s", seq, mName.c_str());
    }
}

void IpcTransactor::dispatchFunctionCallToIpcObject(LpcFunctionHandleContext context) {
    BaseIpcObject &obj = *context.obj;
    IpcTransactor &transactor = *context.transactor;
    const SharedBuffer &buffer = context.buffer;
    LpcEnv env = {&transactor, &buffer};
    uint32_t proxyId = obj.getProxyId();
    uint32_t funcId = buffer.at<LpcTransactionHeader>(0)->funcId;
    uint32_t seq = buffer.at<LpcTransactionHeader>(0)->sequence;
    ArgList argList;
    if (buffer.size() > sizeof(LpcTransactionHeader)) {
        argList = ArgList(buffer.at<char>(sizeof(LpcTransactionHeader)),
                          buffer.size() - sizeof(LpcTransactionHeader));
    }
    LpcResult result;
    int errcode;
    if (argList.isValid()) {
        // dispatch the function call to IPC object
        if (obj.dispatchLpcInvocation(env, result, funcId, argList)) {
            // the function call is handled by IPC object
            if (!result.isValid()) {
                LOGW("LpcHandle NOT return... funcId=%d", funcId);
                result.returnVoid();
            }
            errcode = transactor.sendLpcResponse(seq, proxyId, result);
        } else {
            // no such function
            errcode = transactor.sendLpcResponseError(seq, proxyId, LpcErrorCode::ERR_UNKNOWN_REQUEST);
        }
    } else {
        errcode = transactor.sendLpcResponseError(seq, proxyId, LpcErrorCode::ERR_BAD_REQUEST);
    }
    if (errcode != 0) {
        LOGE("send resp error=%d when handle seq=%d, funcId=%d", errcode, seq, funcId);
    }
}

void IpcTransactor::dispatchEventToIpcObject(EventHandleContext context) {
    BaseIpcObject &obj = *context.obj;
    IpcTransactor &transactor = *context.transactor;
    const SharedBuffer &buffer = context.buffer;
    LpcEnv env = {&transactor, &buffer};
    uint32_t eventId = buffer.at<EventTransactionHeader>(0)->eventId;
    ArgList argList;
    if (buffer.size() > sizeof(EventTransactionHeader)) {
        argList = ArgList(buffer.at<char>(sizeof(EventTransactionHeader)),
                          buffer.size() - sizeof(EventTransactionHeader));
    }
    if (argList.isValid()) {
        // dispatch the event to IPC object, since it is an event, we do not care the result
        (void) obj.dispatchEvent(env, eventId, argList);
    }
}

bool IpcTransactor::registerIpcObject(BaseIpcObject &obj) {
    uint32_t id = obj.getProxyId();
    return mIpcObjects.putIfAbsent(id, &obj);
}

bool IpcTransactor::unregisterIpcObject(uint32_t proxyId) {
    return mIpcObjects.remove(proxyId);
}

bool IpcTransactor::hasIpcObject(uint32_t proxyId) const {
    return mIpcObjects.containsKey(proxyId);
}

std::vector<BaseIpcObject *> IpcTransactor::getRegisteredIpcObjects() const {
    std::vector<BaseIpcObject *> objects;
    for (const auto &it: mIpcObjects.entrySet()) {
        objects.push_back(*it->getValue());
    }
    return objects;
}

void IpcTransactor::unregisterAllIpcObjects() {
    mIpcObjects.clear();
}
