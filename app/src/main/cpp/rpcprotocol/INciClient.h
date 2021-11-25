//
// Created by kinit on 2021-06-09.
//

#ifndef NCI_HOST_INCICLIENT_H
#define NCI_HOST_INCICLIENT_H

#include <string>

#include "libbasehalpatch/ipc/daemon_ipc_struct.h"
#include "protocol/ArgList.h"
#include "protocol/LpcResult.h"

namespace ipcprotocol {

class INciClient {
public:
    static constexpr uint32_t PROXY_ID =
            uint32_t('N') | (uint32_t('C') << 8) | (uint32_t('L') << 16) | (uint32_t('0') << 24);

    INciClient() = default;

    virtual ~INciClient() = default;

    virtual void onIoEvent(const halpatch::IoOperationEvent &event, const std::vector<uint8_t> &payload) = 0;

    virtual void onRemoteDeath(int pid) = 0;

    class EventIds {
    public:
        static constexpr uint32_t IO_EVENT = 0x10001;
        static constexpr uint32_t REMOTE_DEATH = 0x10002;
    };
};

}

#endif //NCI_HOST_INCICLIENT_H
