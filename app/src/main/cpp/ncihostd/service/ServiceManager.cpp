//
// Created by kinit on 2021-11-20.
//

#include <algorithm>
#include "ServiceManager.h"

using namespace hwhal;

ServiceManager &ServiceManager::getInstance() {
    static ServiceManager *volatile sInstance = nullptr;
    if (sInstance == nullptr) {
        // sInstance will NEVER be released
        auto *p = new ServiceManager();
        sInstance = p;
    }
    return *sInstance;
}

std::vector<IBaseService *> ServiceManager::getRunningServices() const {
    return mRunningServices;
}

std::vector<IBaseService *> ServiceManager::findServiceByName(std::string_view name) const {
    std::vector<IBaseService *> result;
    std::scoped_lock lock(mLock);
    for (auto *service: mRunningServices) {
        if (service->getName() == name) {
            result.push_back(service);
        }
    }
    return result;
}

size_t ServiceManager::getRunningServiceCount() const {
    return mRunningServices.size();
}

bool ServiceManager::stopService(IBaseService *service) {
    std::scoped_lock lock(mLock);
    if (auto it = std::find(mRunningServices.begin(), mRunningServices.end(), service);
            it != mRunningServices.end()) {
        if (service->onStop()) {
            mRunningServices.erase(it);
            return true;
        }
    }
    return false;
}

bool ServiceManager::hasService(const IBaseService *service) const {
    std::scoped_lock lock(mLock);
    return std::find(mRunningServices.begin(), mRunningServices.end(), service) != mRunningServices.end();
}
