//
// Created by kinit on 2021-10-24.
//

#ifndef NCI_HOST_NATIVES_INTERCEPT_CALLBACKS_H
#define NCI_HOST_NATIVES_INTERCEPT_CALLBACKS_H

#ifdef __cplusplus
extern "C" {
#endif

void invokeReadResultCallback(ssize_t result, int fd, const void *buffer, size_t size);

void invokeWriteResultCallback(ssize_t result, int fd, const void *buffer, size_t size);

void invokeOpenResultCallback(int result, const char *name, int flags, uint32_t mode);

void invokeCloseResultCallback(int result, int fd);

void invokeIoctlResultCallback(int result, int fd, unsigned long int request, uint64_t arg);

void invokeSelectResultCallback(int result);

#ifdef __cplusplus
};
#endif

#endif //NCI_HOST_NATIVES_INTERCEPT_CALLBACKS_H
