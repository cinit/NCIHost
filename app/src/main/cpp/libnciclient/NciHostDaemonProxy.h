//
// Created by kinit on 2021-10-15.
//

#ifndef NCICLIENT_NCIHOSTDAEMONPROXY_H
#define NCICLIENT_NCIHOSTDAEMONPROXY_H

#include <memory>

#include "../rpcprotocol/IpcProxy.h"
#include "../rpcprotocol/INciHostDaemon.h"

namespace ipcprotocol {

class NciHostDaemonProxy : public INciHostDaemon {
public:
    NciHostDaemonProxy();

    explicit NciHostDaemonProxy(IpcProxy *ipcProxy);

    ~NciHostDaemonProxy() override;

    [[nodiscard]]
    IpcProxy *getIpcProxy() const noexcept;

    void setIpcProxy(IpcProxy *ipcProxy);

    [[nodiscard]]
    bool isConnected() const noexcept;

    TypedLpcResult<std::string> getVersionName() override;

    TypedLpcResult<int> getVersionCode() override;

    TypedLpcResult<std::string> getBuildUuid() override;

    TypedLpcResult<void> exitProcess() override;

    TypedLpcResult<int> testFunction(int value) override;

private:
    IpcProxy *mProxy = nullptr;

    template<class R>
    static inline TypedLpcResult<R> throwBrokenConnectionErrorInternal() {
        TypedLpcResult<R> result;
        result.setError(LpcErrorCode::ERR_BROKEN_CONN);
        return result;
    }

    template<class R, class ...Args>
    inline TypedLpcResult<R> invokeRemoteProcedureInternal(uint32_t funcId, const Args &...args) {
        return mProxy ? TypedLpcResult<R>((mProxy->executeRemoteProcedure(funcId, args...)))
                      : throwBrokenConnectionErrorInternal<R>();
    }
};

}

#endif //NCICLIENT_NCIHOSTDAEMONPROXY_H
