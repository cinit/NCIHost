//
// Created by kinit on 2021-11-20.
//

#ifndef NCI_HOST_NATIVES_SERVICEMANAGER_H
#define NCI_HOST_NATIVES_SERVICEMANAGER_H

#include <vector>
#include <tuple>
#include <mutex>
#include <memory>

#include "IBaseService.h"

namespace hwhal {

class ServiceManager {
public:
    ServiceManager() = default;

    ~ServiceManager() = default;

    ServiceManager(const ServiceManager &) = delete;

    ServiceManager &operator=(const ServiceManager &) = delete;

    [[nodiscard]]
    static ServiceManager &getInstance();

    [[nodiscard]]
    std::vector<std::weak_ptr<IBaseService>> getRunningServices() const;

    [[nodiscard]]
    std::vector<std::shared_ptr<IBaseService>> findServiceByName(std::string_view name) const;

    /**
     * Stop the service and remove it from the list of running services.
     * Note that a service may refuse to stop if it is in a state where it cannot be stopped.
     * @param service the service to stop
     * @return true if the service was stopped, false otherwise
     * @see IBaseService::onStop()
     */
    bool stopService(IBaseService *service);

    size_t getRunningServiceCount() const;

    bool hasService(const IBaseService *service) const;

private:
    mutable std::mutex mLock;
    std::vector<std::shared_ptr<IBaseService>> mRunningServices;

public:
    /**
     * Start target service.
     * @tparam ServiceType the service to start.
     * @param args optional arguments to pass to the service.
     * @return [result, wpService], result is non-negative on success, negative on failure.
     */
    template<typename ServiceType>
    [[nodiscard]]
    std::tuple<int, std::weak_ptr<IBaseService>> startService(void *args) {
        // do not use std::make_shared here, type mismatch
        std::shared_ptr<IBaseService> sp((IBaseService *) (new ServiceType()));
        int result = sp->onStart(args, sp);
        if (result < 0) {
            // sp is automatically destroyed
            return std::make_tuple(result, std::weak_ptr<IBaseService>());
        }
        std::scoped_lock lock(mLock);
        mRunningServices.push_back(sp);
        return std::make_tuple(result, sp);
    }
};

}

#endif //NCI_HOST_NATIVES_SERVICEMANAGER_H
