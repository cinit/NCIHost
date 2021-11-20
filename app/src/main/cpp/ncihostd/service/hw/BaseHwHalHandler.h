//
// Created by kinit on 2021-11-17.
//

#ifndef NCI_HOST_NATIVES_BASEHWHALHANDLER_H
#define NCI_HOST_NATIVES_BASEHWHALHANDLER_H

#include <atomic>
#include <string>
#include <pthread.h>

#include "libbasehalpatch/ipc/daemon_ipc_struct.h"
#include "../IBaseService.h"

namespace hwhal {

class BaseHwHalHandler : public IBaseService {
public:
    struct StartupInfo {
        int fd;
        int pid;
    };

    BaseHwHalHandler() = default;

    ~BaseHwHalHandler() override = default;

    [[nodiscard]] int onStart(void *args) override;

    [[nodiscard]] bool onStop() override;

    [[nodiscard]] virtual int doOnStart(void *args) = 0;

    [[nodiscard]] virtual bool doOnStop() = 0;

    void stopSelf();

private:
    std::atomic<bool> mIsRunning = false;
    int mRemotePid = -1;
    pthread_t mThread = 0;
    int mFd = -1;

    static void workerThread(BaseHwHalHandler *self);

    void dispatchHwHalPatchIpcPacket(const void *packet, size_t length);

protected:
    virtual void dispatchHwHalIoEvent(const IoOperationEvent &event, const void *payload) = 0;

    virtual void dispatchRemoteProcessDeathEvent() = 0;

public:
    [[nodiscard]] inline int getFd() const {
        return mFd;
    }

    [[nodiscard]] inline int getRemotePid() const {
        return mRemotePid;
    }

};

}

#endif //NCI_HOST_NATIVES_BASEHWHALHANDLER_H
