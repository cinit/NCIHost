//
// Created by kinit on 2021-11-18.
//

#ifndef NCI_HOST_NATIVES_IPC_REQUESTS_H
#define NCI_HOST_NATIVES_IPC_REQUESTS_H

#include <cstdint>

enum class RequestId : uint32_t {
    GET_VERSION = 0x1,
    GET_HOOK_STATUS = 0x40,
    INIT_PLT_HOOK = 0x41,
    GET_DEVICE_STATUS = 0x50,
    DEVICE_OPERATION_WRITE = 0x51,
    DEVICE_OPERATION_IOCTL0 = 0x52,
};

struct NxpNciDeviceStatus {
    static constexpr uint8_t STATUS_UNKNOWN = 0;
    static constexpr uint8_t STATUS_ENABLED = 1;
    static constexpr uint8_t STATUS_DISABLED = 2;

    uint8_t nfccPowerStatus;
    uint8_t esePowerStatus;
    uint8_t unused1_0;
    uint8_t unused1_1;
    uint32_t deviceFileDescriptor;
    uint32_t unused4_2;
    uint32_t unused4_3;
};
static_assert(sizeof(NxpNciDeviceStatus) == 16, "NxpNciDeviceStatus size error");

struct DeviceIoctl0Request {
    uint64_t request;
    uint64_t arg;
};
static_assert(sizeof(DeviceIoctl0Request) == 16, "DeviceIoctl0Request size error");

struct DeviceWriteRequest {
    uint32_t length;
    uint8_t data[];
};
// no size static assert...

#endif //NCI_HOST_NATIVES_IPC_REQUESTS_H
