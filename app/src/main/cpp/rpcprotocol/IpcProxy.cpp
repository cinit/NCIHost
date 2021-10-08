//
// Created by kinit on 2021-10-02.
//

#include <unistd.h>
#include <cerrno>
#include <sstream>
#include <sys/fcntl.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "rpc_struct.h"
#include "IpcProxy.h"
#include "Log.h"

#define LOG_TAG "IpcProxy"

IpcProxy::IpcProxy() = default;

IpcProxy::~IpcProxy() {
    shutdown();
}

void IpcProxy::setEventHandler(IpcProxy::EventHandler h) {
    std::scoped_lock<std::mutex> _(mStatusLock);
    mEventHandler = h;
}

void IpcProxy::setFunctionHandler(IpcProxy::LpcFuncHandler h) {
    std::scoped_lock<std::mutex> _(mStatusLock);
    mFuncHandler = h;
}

int IpcProxy::attach(int fd) {
    std::scoped_lock<std::mutex> lock(mStatusLock);
    if (fd < 0) {
        return EBADFD;
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

void IpcProxy::join() {
    {
        std::scoped_lock<std::mutex> wait_if_shutdown(mStatusLock);
    }
    std::unique_lock lk(mJoinWaitCondVarMutex);
    if (mIsRunning) {
        mJoinWaitCondVar.wait(lk);
    }
    {
        std::scoped_lock<std::mutex> wait_if_shutdown(mStatusLock);
    }
}

void IpcProxy::start() {
    {
        std::scoped_lock<std::mutex> lk(mStatusLock);
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
    }
    std::unique_lock startLock(mRunningEntryMutex);
    pthread_t tid = 0;
    int result = pthread_create(&tid, nullptr,
                                reinterpret_cast<void *(*)(void *)>(&IpcProxy::runIpcLooperProc), this);
    if (result != 0) {
        char buf[64];
        snprintf(buf, 64, "create thread failed errno=%d", result);
        LOGE("create thread failed errno=%d", result);
        throw std::runtime_error(buf);
    }
    mRunningEntryStartCondVar.wait(startLock);
}

int IpcProxy::detach() {
    std::scoped_lock<std::mutex> lk(mStatusLock);
    return detachLocked();
}

bool IpcProxy::interrupt() {
    std::scoped_lock<std::mutex> lk(mStatusLock);
    return interruptLocked();
}

void IpcProxy::shutdown() {
    std::scoped_lock<std::mutex> lk(mStatusLock);
    shutdownLocked();
}

int IpcProxy::detachLocked() {
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

bool IpcProxy::interruptLocked() {
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

void IpcProxy::shutdownLocked() {
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

void IpcProxy::notifyWaitingCalls() {
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

void IpcProxy::runIpcLooper() {
    std::scoped_lock<std::mutex> runLock(mReadThreadMutex);
    {
        std::scoped_lock<std::mutex> lk(mRunningEntryMutex);
        pthread_setname_np(pthread_self(), "IPC-trxn");
        mRunningEntryStartCondVar.notify_all();
    }
    if (mSocketFd >= 0 && mInterruptEventFd >= 0) {
        SharedBuffer sharedBuffer;
        if (!sharedBuffer.ensureCapacity(65536, std::nothrow_t())) {
            throw std::bad_alloc();
        }
        void *buffer = sharedBuffer.get();
        int epollFd = epoll_create(2);
        {
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
        }
        LOGI("pid=%d, mSocketFd=%d, mInterruptEventFd=%d %s", getpid(), mSocketFd, mInterruptEventFd, mName.c_str());
        mIsRunning = true;
        while (mSocketFd != -1 && mInterruptEventFd != -1) {
            epoll_event event = {};
            if (epoll_wait(epollFd, &event, 1, -1) < 0) {
                int err = errno;
                LOGE("epoll_wait error: %d %s", err, mName.c_str());
                if (err != EINTR) {
                    abort();
                    break;
                }
            } else {
                LOGE("epoll_wait return ev=%d fd=%d %s", event.events, event.data.fd, mName.c_str());
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
                    LOGI("epoll: socket fd, %d %s", event.events, mName.c_str());
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
                    *(volatile int *) 0 = 0;
                    abort();
                }
            }
        }
        mIsRunning = false;
        close(epollFd);
        mJoinWaitCondVar.notify_all();
    }
}

int IpcProxy::sendRawPacket(const void *buffer, size_t size) {
    std::scoped_lock<std::mutex> lk(mTxLock);
    LOGI("send size %zu %s", size, mName.c_str());
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

int IpcProxy::sendLpcRequest(uint32_t sequence, uint32_t funId, const SharedBuffer &args) {
    SharedBuffer result;
    if (!result.ensureCapacity(sizeof(LpcTransactionHeader) + args.size(), std::nothrow_t())) {
        throw std::bad_alloc();
    }
    memcpy(result.at<char>(sizeof(LpcTransactionHeader)), args.get(), args.size());
    auto *h = result.at<LpcTransactionHeader>(0);
    h->header.length = (uint32_t) result.size();
    h->header.type = TrxnType::TRXN_TYPE_RPC;
    h->funcId = funId;
    h->sequence = sequence;
    h->rpcFlags = 0;
    h->errorCode = 0;
    return sendRawPacket(result.get(), result.size());
}

int IpcProxy::sendLpcResponse(uint32_t sequence, const LpcResult &result) {
    SharedBuffer buf = result.buildLpcResponsePacket(sequence);
    if (buf.get() != nullptr) {
        return sendRawPacket(buf.get(), buf.size());
    } else {
        return EINVAL;
    }
}

int IpcProxy::sendLpcResponseError(uint32_t sequence, LpcErrorCode errorId) {
    SharedBuffer buf;
    buf.ensureCapacity(sizeof(LpcTransactionHeader));
    auto *h = buf.at<LpcTransactionHeader>(0);
    h->header.type = sizeof(LpcTransactionHeader);
    h->header.type = TRXN_TYPE_RPC;
    h->errorCode = static_cast<uint32_t>(errorId);
    h->sequence = sequence;
    h->rpcFlags = LPC_FLAG_ERROR;
    h->funcId = 0;
    return sendRawPacket(buf.get(), buf.size());
}

int IpcProxy::sendEvent(uint32_t sequence, uint32_t eventId, const SharedBuffer &args) {
    SharedBuffer result;
    if (!result.ensureCapacity(sizeof(EventTransactionHeader) + args.size(), std::nothrow_t())) {
        throw std::bad_alloc();
    }
    memcpy(result.at<char>(sizeof(EventTransactionHeader)), args.get(), args.size());
    auto *h = result.at<EventTransactionHeader>(0);
    h->header.length = (uint32_t) result.size();
    h->header.type = TrxnType::TRXN_TYPE_EVENT;
    h->eventId = eventId;
    h->sequence = sequence;
    h->eventFlags = 0;
    h->reserved = 0;
    return sendRawPacket(result.get(), result.size());
}

LpcResult IpcProxy::executeLpcTransaction(uint32_t funcId, const SharedBuffer &args) {
    LpcResult result;
    uint32_t seq = mCurrentSequence.fetch_add(1);
    SharedBuffer buffer;
    std::mutex mtx;
    std::condition_variable condvar;
    LpcReturnStatus returnStatus = {&condvar, &buffer, 0};
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
        mWaitingMap.put(seq, &returnStatus);
    }
    {
        std::unique_lock retlk(mtx);
        if (sendLpcRequest(seq, funcId, args) != 0) {
            mWaitingMap.remove(seq);
            result.setError(LpcErrorCode::ERR_LOCAL_INTERNAL_ERROR);
            return result;
        }
        condvar.wait(retlk);
    }
    mWaitingMap.remove(seq);
    if (buffer.get() != nullptr) {
        return LpcResult::fromLpcRespPacketBuffer(buffer);
    } else {
        result.setError((LpcErrorCode) returnStatus.error);
    }
    return result;
}

void IpcProxy::handleReceivedPacket(const void *buffer, size_t size) {
    if (size < 24) {
        LOGE("packet too small(%zu), ignore  %s", size, mName.c_str());
        return;
    }
    if (size > 65536) {
        LOGE("packet too large(%zu), ignore %s", size, mName.c_str());
        return;
    }
    const auto *header = reinterpret_cast<const TrxnPacketHeader *>(buffer);
    uint32_t type = header->type;
    if (type == TRXN_TYPE_RPC) {
        if (reinterpret_cast<const LpcTransactionHeader *>(buffer)->funcId == 0) {
            handleLpcResponsePacket(buffer, size);
        } else {
            dispatchLpcRequestAsync(buffer, size);
        }
    } else if (type == TRXN_TYPE_EVENT) {
        dispatchEventPacketAsync(buffer, size);
    } else {
        LOGE("unknown type (%ud), ignore %s", type, mName.c_str());
    }
}

void IpcProxy::dispatchEventPacketAsync(const void *buffer, size_t size) {
    EventHandler h = mEventHandler;
    if (h != nullptr) {
        SharedBuffer sb;
        if (!sb.ensureCapacity(size, std::nothrow_t())) {
            LOGE("failed to allocate buffer size = %zu, drop event  %s", size, mName.c_str());
            return;
        }
        memcpy(sb.get(), buffer, size);
        EventHandleContext context = {h, this, sb};
        mExecutor.execute([context]() {
            IpcProxy::invokeEventHandler(context);
        });
    }
}

void IpcProxy::dispatchLpcRequestAsync(const void *buffer, size_t size) {
    LpcFuncHandler h = mFuncHandler;
    uint32_t seq = reinterpret_cast<const LpcTransactionHeader *>(buffer)->sequence;
    if (h != nullptr) {
        SharedBuffer sb;
        if (!sb.ensureCapacity(size, std::nothrow_t())) {
            sendLpcResponseError(seq, LpcErrorCode::ERR_REMOTE_INTERNAL_ERROR);
            LOGE("failed to allocate buffer size = %zu, return lpc error  %s", size, mName.c_str());
            return;
        }
        memcpy(sb.get(), buffer, size);
        LpcFunctHandleContext context = {h, this, sb};
        mExecutor.execute([context]() {
            IpcProxy::invokeLpcHandler(context);
        });
    } else {
        sendLpcResponseError(seq, LpcErrorCode::ERR_NO_LPC_HANDLER);
    }
}

void IpcProxy::handleLpcResponsePacket(const void *buffer, size_t size) {
    uint32_t seq = reinterpret_cast<const LpcTransactionHeader *>(buffer)->sequence;
    LpcReturnStatus **pStatus = mWaitingMap.get(seq);
    if (pStatus != nullptr) {
        LpcReturnStatus *status = *pStatus;
        if (status->buffer->resetSize(size, false)) {
            memcpy(status->buffer->get(), buffer, size);
            status->error = 0;
        } else {
            status->error = static_cast<uint32_t>(LpcErrorCode::ERR_LOCAL_INTERNAL_ERROR);
        }
        status->cond->notify_all();
    } else {
        LOGE("handleLpcResponsePacket but no LpcReturnStatus found, seq=%ud  %s", seq, mName.c_str());
    }
}

void IpcProxy::invokeLpcHandler(LpcFunctHandleContext context) {
    LpcEnv env = {context.object, &context.buffer};
    SharedBuffer *sb = &context.buffer;
    IpcProxy *that = context.object;
    LpcFuncHandler h = context.h;
    uint32_t funcId = sb->at<LpcTransactionHeader>(0)->funcId;
    uint32_t seq = sb->at<LpcTransactionHeader>(0)->sequence;
    ArgList argList;
    if (sb->size() > 24) {
        argList = ArgList(sb->at<char>(sizeof(LpcTransactionHeader)),
                          sb->size() - sizeof(LpcTransactionHeader));
    }
    LpcResult result;
    int errcode = 0;
    if (argList.isValid()) {
        if (h != nullptr) {
            if (h(env, result, funcId, argList)) {
                if (!result.isValid()) {
                    LOGW("LpcHandle NOT return... funcId=%d", funcId);
                    result.returnVoid();
                }
                errcode = that->sendLpcResponse(seq, result);
            } else {
                errcode = that->sendLpcResponseError(seq, LpcErrorCode::ERR_INVALID_FUNCTION);
            }
        } else {
            errcode = that->sendLpcResponseError(seq, LpcErrorCode::ERR_NO_LPC_HANDLER);
        }
    } else {
        errcode = that->sendLpcResponseError(seq, LpcErrorCode::ERR_BAD_BUFFER);
    }
    if (errcode != 0) {
        LOGE("send resp error=%d when handle seq=%d, funcId=%d", errcode, seq, funcId);
    }
}

void IpcProxy::invokeEventHandler(EventHandleContext context) {
    LpcEnv env = {context.object, &context.buffer};
    SharedBuffer *sb = &context.buffer;
    EventHandler h = context.h;
    uint32_t eventId = sb->at<LpcTransactionHeader>(0)->funcId;
    uint32_t seq = sb->at<LpcTransactionHeader>(0)->sequence;
    ArgList argList;
    if (sb->size() > 24) {
        argList = ArgList(sb->at<char>(sizeof(EventTransactionHeader)),
                          sb->size() - sizeof(EventTransactionHeader));
    }
    LpcResult result;
    if (argList.isValid()) {
        if (h != nullptr) {
            h(env, eventId, argList);
        }
    } else {
        LOGE("bad event seq=%d, event_type=%d", seq, eventId);
    }
}
