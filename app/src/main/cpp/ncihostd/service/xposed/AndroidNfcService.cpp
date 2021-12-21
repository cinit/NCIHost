//
// Created by kinit on 2021-12-20.
//

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "rpcprotocol/utils/ProcessUtils.h"
#include "rpcprotocol/log/Log.h"

#include "AndroidNfcService.h"

using namespace androidsvc;

const char *const LOG_TAG = "AndroidNfcService";

AndroidNfcService &AndroidNfcService::getInstance() {
    static AndroidNfcService slgInstance;
    return slgInstance;
}

bool AndroidNfcService::tryConnect() {
    if (isConnected()) {
        return true;
    }
    // find nfc service pid
    int pid = -1;
    auto processList = utils::getRunningProcessInfo();
    for (const auto &process: processList) {
        if (process.argv0 == "com.android.nfc" && process.uid >= 1000 && process.uid < 10000) {
            pid = process.pid;
            break;
        }
    }
    if (pid == -1) {
        LOGE("com.android.nfc service not found");
        return false;
    } else {
        LOGD("com.android.nfc service pid: %d", pid);
    }
    std::string socketName = "com.android.nfc/cc.ioctl.nfcdevicehost.xposed.SysNfcSvcPatch@" + std::to_string(pid);
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        LOGE("socket failed, errno: %d", errno);
        return false;
    }
    struct sockaddr_un addr = {};
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path + 1, socketName.c_str(), sizeof(addr.sun_path) - 1);
    socklen_t addrLen = sizeof(addr.sun_family) + 1 + strlen(addr.sun_path + 1);
    if (connect(sock, (struct sockaddr *) &addr, addrLen) == -1) {
        LOGE("connect failed, errno: %d", errno);
        close(sock);
        return false;
    }
    // check peer credentials
    ucred credentials = {};
    if (socklen_t len = sizeof(credentials); getsockopt(sock, SOL_SOCKET, SO_PEERCRED, &credentials, &len) == -1) {
        LOGE("getsockopt SO_PEERCRED failed, errno: %d", errno);
        close(sock);
        return false;
    }
    if (pid != credentials.pid) {
        LOGE("invalid credentials, pid: %d, uid: %d", credentials.pid, credentials.uid);
        close(sock);
        return false;
    }
    mNfcServicePid = pid;
    int err = startWorkerThread(sock);
    if (err != 0) {
        LOGE("startWorkerThread failed, errno: %d", err);
        close(sock);
        return false;
    }
    return true;
}

bool AndroidNfcService::isConnected() const noexcept {
    return getSocketFd() != -1 && mNfcServicePid > 0 && getWorkerThread() > 0;
}

int AndroidNfcService::getNfcServicePid() const noexcept {
    return mNfcServicePid;
}

bool AndroidNfcService::isNfcDiscoverySoundDisabled() {
    if (!isConnected()) {
        return false;
    }
    RequestPacket request = {REQUEST_GET_NFC_SOUND_DISABLE, 0};
    auto[err, response] = sendRequest(request, 1000);
    if (err != 0) {
        LOGE("sendRequest failed, errno: %d", err);
        return false;
    }
    return response.resultCode != 0;
}

bool AndroidNfcService::setNfcDiscoverySoundDisabled(bool disabled) {
    if (!isConnected()) {
        return false;
    }
    RequestPacket request = {REQUEST_SET_NFC_SOUND_DISABLE, uint32_t(disabled ? 1 : 0)};
    auto[err, response] = sendRequest(request, 1000);
    if (err != 0) {
        LOGE("sendRequest failed, errno: %d", err);
        return false;
    }
    return response.errorCode == 0;
}

void AndroidNfcService::dispatchServiceEvent(const BaseRemoteAndroidService::EventPacket &event) {
    LOGE("unsupported event: %d", event.eventCode);
}

void AndroidNfcService::dispatchConnectionBroken() {
    mNfcServicePid = 0;
}
