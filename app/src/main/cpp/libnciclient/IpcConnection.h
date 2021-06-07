//
// Created by kinit on 2021-05-15.
//

#ifndef NCI_HOST_IPCCONNECTION_H
#define NCI_HOST_IPCCONNECTION_H

#include <cstdint>

#define IPC_SHM_SIZE (1*1024*1024)

typedef struct {
    uint32_t pid;
    char name[108];
} IpcSocketMeta;

//typedef struct {
//    uint32_t packetVersion;
//    uint32_t packetSize;
//    uint32_t packetType;
//
//    uint32_t packetId;
////    uint32_t packetId;
//    char name[108];
//} IpcSocketMeta;

static_assert(sizeof(IpcSocketMeta) == 112, "IpcSocketMeta size error");

#endif //NCI_HOST_IPCCONNECTION_H
