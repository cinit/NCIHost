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

std::vector<std::weak_ptr<IBaseService>> ServiceManager::getRunningServices() const {
    std::scoped_lock lock(mLock);
    std::vector<std::weak_ptr<IBaseService>> resultList;
    resultList.reserve(mRunningServices.size());
    for (auto &s: mRunningServices) {
        resultList.push_back(s);
    }
    return resultList;
}

std::vector<std::shared_ptr<IBaseService>> ServiceManager::findServiceByName(std::string_view name) const {
    std::vector<std::shared_ptr<IBaseService>> result;
    std::scoped_lock lock(mLock);
    for (auto &service: mRunningServices) {
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
    if (auto it = std::find_if(mRunningServices.begin(), mRunningServices.end(),
                               [&](const auto &sp) { return sp.get() == service; });
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
    return std::find_if(mRunningServices.begin(), mRunningServices.end(),
                        [&](const auto &sp) { return sp.get() == service; }) != mRunningServices.end();
}
