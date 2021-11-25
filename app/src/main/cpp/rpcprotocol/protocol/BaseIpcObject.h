//
// Created by kinit on 2021-11-22.
//

#ifndef NCI_HOST_NATIVES_BASEIPCOBJECT_H
#define NCI_HOST_NATIVES_BASEIPCOBJECT_H

#include <cstdint>

#include "ArgList.h"
#include "LpcResult.h"
#include "IpcTransactor.h"

namespace ipcprotocol {

class IpcTransactor;

class BaseIpcObject {
public:
    BaseIpcObject() = default;

    virtual ~BaseIpcObject() = default;

    [[nodiscard]] virtual uint32_t getProxyId() const = 0;

    /**
     * Handle a remote procedure call.
     * @param env the transaction environment.
     * @param result the result of the call.
     * @param funcId the id of the function.
     * @param args arguments of the function.
     * @return true if the call is handled, false if there is no such function.
     */
    virtual bool dispatchLpcInvocation(const IpcTransactor::LpcEnv &env, LpcResult &result,
                                       uint32_t funcId, const ArgList &args) = 0;

    /**
     * Handle an event from the remote proxy.
     * @param env the transaction environment.
     * @param eventId the id of the event.
     * @param args arguments of the event.
     * @return true if the event is handled, false if there is no such event.
     */
    virtual bool dispatchEvent(const IpcTransactor::LpcEnv &env, uint32_t eventId, const ArgList &args) = 0;
};

}

#endif //NCI_HOST_NATIVES_BASEIPCOBJECT_H
