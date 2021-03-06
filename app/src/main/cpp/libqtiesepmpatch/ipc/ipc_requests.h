//
// Created by kinit on 2021-11-18.
//

#ifndef NCI_HOST_NATIVES_IPC_REQUESTS_H
#define NCI_HOST_NATIVES_IPC_REQUESTS_H

#include <cstdint>

namespace halpatch::qtiesepm {

enum class RequestId : uint32_t {
    GET_VERSION = 0x1,
    GET_HOOK_STATUS = 0x40,
    INIT_PLT_HOOK = 0x41,
    GET_NCI_FD = 0x61,
};

}

#endif //NCI_HOST_NATIVES_IPC_REQUESTS_H
