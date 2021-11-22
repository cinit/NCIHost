//
// Created by kinit on 2021-11-18.
//

#ifndef NCI_HOST_NATIVES_REQUEST_HANDLER_H
#define NCI_HOST_NATIVES_REQUEST_HANDLER_H

#include "daemon_ipc_struct.h"

/**
 * handle a platform specific request from the daemon,
 * this function is implemented by the platform specific hw-service-patch module
 * @param request request from the daemon
 * @param payload extra request data, may be NULL if request->payload_size is 0
 * @return non-negative value if success, negative value if failed
 */
int handleRequestPacket(const halpatch::HalRequest &request, const void *payload);

/**
 * send response back to the daemon
 * @param response the result of the request
 * @param payload extra data, may be NULL if response->payload_size is 0
 * @return 0 on success, negative value on failure
 */
int sendResponsePacket(const halpatch::HalResponse &response, const void *payload);

/**
 * send error response to the daemon
 * @param requestId the request id
 * @param errorCode error code, @see HalRequestErrorCode
 * @param errMsg detailed error message, may be NULL
 * @return 0 on success, negative value on failure
 */
int sendResponseError(uint32_t requestId, halpatch::HalRequestErrorCode errorCode, const char *errMsg);

#endif //NCI_HOST_NATIVES_REQUEST_HANDLER_H
