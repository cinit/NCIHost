//
// Created by kinit on 2021-12-20.
//

#ifndef NCI_HOST_NATIVES_ANDROIDNFCSERVICE_H
#define NCI_HOST_NATIVES_ANDROIDNFCSERVICE_H

#include <mutex>

#include "BaseRemoteAndroidService.h"

namespace androidsvc {

class AndroidNfcService : public BaseRemoteAndroidService {
private:
    AndroidNfcService() = default;

public:
    using BaseRemoteAndroidService::ResponsePacket;
    using BaseRemoteAndroidService::RequestPacket;
    using BaseRemoteAndroidService::EventPacket;

    constexpr static int REQUEST_SET_NFC_SOUND_DISABLE = 0x30;
    constexpr static int REQUEST_GET_NFC_SOUND_DISABLE = 0x31;

    ~AndroidNfcService() override = default;

    AndroidNfcService(const AndroidNfcService &) = delete;

    AndroidNfcService &operator=(const AndroidNfcService &) = delete;

    /**
     * Get the singleton instance of AndroidNfcService.
     * @return the instance
     */
    [[nodiscard]] static AndroidNfcService &getInstance();

    /**
     * Try to connect to the Android NFC service.
     * @return true if the connection was successful, false otherwise
     */
    [[nodiscard]] bool tryConnect();

    /**
     * Check if the Android NFC service is connected to the daemon.
     * @return true if the service is connected, false otherwise.
     */
    [[nodiscard]] bool isConnected() const noexcept;

    /**
     * Get the process id of the Android NFC service.
     * @return the process id of the service, or -1 if the service is not connected.
     */
    [[nodiscard]] int getNfcServicePid() const noexcept;

    /**
     * Check if the NFC will play a sound when a tag is detected.
     * @return true if the NFC service is configured not to play a sound.
     */
    [[nodiscard]] bool isNfcDiscoverySoundDisabled();

    /**
     * Set the NFC discovery sound disable flag.
     * @param disabled true to disable the sound.
     * @return true if the flag was set successfully, false otherwise(e.g. if the service is not connected).
     */
    bool setNfcDiscoverySoundDisabled(bool disabled);

private:
    int mNfcServicePid = -1;

protected:
    void dispatchServiceEvent(const EventPacket &event) override;

    void dispatchConnectionBroken() override;

};

}

#endif //NCI_HOST_NATIVES_ANDROIDNFCSERVICE_H
