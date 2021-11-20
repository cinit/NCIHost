//
// Created by kinit on 2021-11-20.
//

#ifndef NCI_HOST_NATIVES_SERVICEMANAGER_H
#define NCI_HOST_NATIVES_SERVICEMANAGER_H

#include <vector>
#include <tuple>
#include <mutex>

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
    std::vector<IBaseService *> getRunningServices() const;

    [[nodiscard]]
    std::vector<IBaseService *> findServiceByName(std::string_view name) const;

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
    std::vector<IBaseService *> mRunningServices;

public:
    /**
     * Start target service.
     * @tparam ServiceType the service to start.
     * @param args optional arguments to pass to the service.
     * @return [result, service], result is non-negative on success, negative on failure.
     */
    template<typename ServiceType>
    [[nodiscard]]
    std::tuple<int, ServiceType *> startService(void *args) {
        auto *service = new ServiceType();
        auto result = service->onStart(args);
        if (result < 0) {
            delete service;
            return std::make_tuple(result, nullptr);
        }
        std::scoped_lock lock(mLock);
        mRunningServices.push_back(service);
        return std::make_tuple(result, service);
    }
};

}

#endif //NCI_HOST_NATIVES_SERVICEMANAGER_H
