//
// Created by kinit on 2021-11-20.
//
#ifndef NCI_HOST_NATIVES_IBASESERVICE_H
#define NCI_HOST_NATIVES_IBASESERVICE_H

#include <memory>
#include <string_view>

namespace hwhal {

/**
 * The base interface for all service that can be registered to the service manager.
 * Note that the deconstruct function must be virtual.
 */
class IBaseService {
public:
    IBaseService() = default;

    virtual ~IBaseService() = default;

    /**
     * Start the service. A service may fail to start for various reasons.
     * @param args The arguments to the service, may be NULL.
     * @return non-negative on success, negative on failure.
     */
    [[nodiscard]] virtual int onStart(void *args, const std::shared_ptr<IBaseService> &sp) = 0;

    /**
     * Called when the service is requested to stop.
     * If this method returns false, the service will not be stopped.
     * If this method returns true, the service will be stopped and instance will be deconstructed.
     * Note that all resources should be released before this method returns true.
     * @return whether the service can be stopped now
     * @see ServiceManager::stopService(IBaseService*)
     */
    [[nodiscard]] virtual bool onStop() = 0;

    /**
     * Get the service name, which should be unique,
     * and remain constant during the life time of the service.
     * @return the service name
     */
    [[nodiscard]] virtual std::string_view getName() const noexcept = 0;

    /**
     * Get the service description, usually including the service current status.
     * @return the service description for the time being
     */
    [[nodiscard]] virtual std::string_view getDescription() const noexcept = 0;

};

}

#endif //NCI_HOST_NATIVES_IBASESERVICE_H
