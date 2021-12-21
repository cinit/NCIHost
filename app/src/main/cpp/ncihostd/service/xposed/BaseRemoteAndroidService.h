//
// Created by kinit on 2021-12-21.
//

#ifndef NCI_HOST_NATIVES_BASEREMOTEANDROIDSERVICE_H
#define NCI_HOST_NATIVES_BASEREMOTEANDROIDSERVICE_H

#include <vector>
#include <string>
#include <mutex>
#include <tuple>
#include <atomic>
#include <condition_variable>

#include "rpcprotocol/utils/HashMap.h"

namespace androidsvc {

class BaseRemoteAndroidService {
public:
    static const int TYPE_REQUEST = 1;
    static const int TYPE_RESPONSE = 2;
    static const int TYPE_EVENT = 3;

    class RequestPacket {
    public:
        uint32_t sequence = 0;
        uint32_t requestCode = 0;
        uint32_t arg0 = 0;
        std::vector<uint8_t> payload;

        inline RequestPacket() = default;

        inline RequestPacket(uint32_t request, uint32_t arg) : requestCode(request), arg0(arg) {}

        [[nodiscard]] std::vector<uint8_t> toByteArray() const;

        [[nodiscard]] std::vector<uint8_t> toByteArray(uint32_t overrideSequence) const;
    };

    class ResponsePacket {
    public:
        uint32_t sequence = 0;
        uint32_t resultCode = 0;
        uint32_t errorCode = 0;
        std::vector<uint8_t> payload;

        [[nodiscard]] std::vector<uint8_t> toByteArray() const;
    };

    class EventPacket {
    public:
        inline EventPacket() = default;

        inline EventPacket(uint32_t seq, uint32_t eventCode, uint32_t arg)
                : sequence(seq), eventCode(eventCode), arg0(arg) {}

        uint32_t sequence = 0;
        uint32_t eventCode = 0;
        uint32_t arg0 = 0;
        std::vector<uint8_t> payload;

        [[nodiscard]] std::vector<uint8_t> toByteArray() const;
    };

    BaseRemoteAndroidService() = default;

    virtual ~BaseRemoteAndroidService() = default;

    /**
     * Send a request to the remote service.
     * @param request the request to send.
     * @param timeout_ms the timeout in milliseconds.
     * @return the tuple of (errno, response). If errno is 0, response is valid.
     */
    [[nodiscard]]
    std::tuple<int, ResponsePacket> sendRequest(const RequestPacket &request, int timeout_ms);

private:
    class LpcReturnStatus {
    public:
        std::condition_variable *cond;
        ResponsePacket *response;
        uint32_t error;
    };

    std::atomic<bool> mIsRunning = false;
    std::atomic<uint32_t> mSequenceId = 0x10000000;
    std::mutex mIpcMutex;
    pthread_t mThread = 0;
    int mFd = -1;
    HashMap<uint32_t, LpcReturnStatus *> mWaitingRequests;

    static void workerThread(BaseRemoteAndroidService *self);

    void dispatchServiceIpcPacket(const void *packet, size_t length);

protected:
    /**
     * Start the worker thread. If an error occurs, the socket file descriptor will be untouched.
     * @param streamSocketFd the socket file descriptor.
     * @return 0 on success, errno on failure.
     */
    int startWorkerThread(int streamSocketFd);

    inline int getSocketFd() const noexcept {
        return mFd;
    }

    inline pthread_t getWorkerThread() const noexcept {
        return mThread;
    }

    virtual void dispatchServiceEvent(const EventPacket &event) = 0;

    virtual void dispatchConnectionBroken() = 0;
};

}

#endif //NCI_HOST_NATIVES_BASEREMOTEANDROIDSERVICE_H
