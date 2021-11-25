//
// Created by kinit on 2021-11-22.
//

#ifndef NCI_HOST_NATIVES_BASEIPCPROXY_H
#define NCI_HOST_NATIVES_BASEIPCPROXY_H

#include "../utils/SharedBuffer.h"
#include "LpcResult.h"

namespace ipcprotocol {

class IpcTransactor;

class BaseIpcProxy {
public:
    BaseIpcProxy() = default;

    virtual ~BaseIpcProxy() = default;

    [[nodiscard]] virtual uint32_t getProxyId() const = 0;

    [[nodiscard]] bool isConnected() const;

    void attachToIpcTransactor(IpcTransactor &ipcTransactor);

    void detachFromIpcTransactor();

    [[nodiscard]] IpcTransactor *getIpcTransactor() const;

private:
    IpcTransactor *mIpcTransactor = nullptr;

protected:
    /**
     * Dispatch a remote invocation on the proxy object to the IPC transactor.
     * @param procId the function id to invoke
     * @param args arguments of the remote procedure call
     * @return the result of the remote invocation
     */
    [[nodiscard]] LpcResult dispatchProxyInvocation(uint32_t procId, const SharedBuffer &args);

    /**
     * Wrap arguments of a remote invocation and dispatch it to the IPC transactor.
     * @tparam R the return type of the remote procedure
     * @tparam Args argument types of the remote procedure
     * @param procId the function id to invoke
     * @param args arguments of the remote procedure call
     * @return the result of the remote invocation
     */
    template<typename R, typename ...Args>
    [[nodiscard]] TypedLpcResult<R> invokeRemoteProcedure(uint32_t procId, Args &...args) {
        LpcResult result = dispatchProxyInvocation(procId, ArgList::Builder().pushArgs(args...).build());
        return TypedLpcResult<R>(result);
    }

    /**
     * Pass an event to the remote object by the IPC transactor.
     * @param sync whether this event should be sent synchronously
     * @param eventId the id of the event
     * @param args event arguments
     * @return 0 if the event was sent successfully, otherwise an error code
     * @see LpcResult::getError(LpcErrorCode)
     */
    [[nodiscard]] LpcErrorCode dispatchProxyEvent(bool sync, uint32_t eventId, const SharedBuffer &args);

    /**
     * Pass an event to the remote object by the IPC transactor.
     * @tparam Args event argument types
     * @param sync whether this event should be sent synchronously
     * @param eventId the id of the event
     * @param args event arguments
     * @return 0 if the event was sent successfully, otherwise an error code
     * @see LpcResult::getError(LpcErrorCode)
     */
    template<typename ...Args>
    [[nodiscard]] LpcErrorCode onProxyEvent(bool sync, uint32_t eventId, Args &...args) {
        return dispatchProxyEvent(sync, eventId, ArgList::Builder().pushArgs(args...).build());
    }
};

}

#endif //NCI_HOST_NATIVES_BASEIPCPROXY_H
