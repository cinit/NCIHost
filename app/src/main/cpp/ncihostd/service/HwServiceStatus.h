//
// Created by kinit on 2021-11-26.
//

#ifndef NCI_HOST_NATIVES_HWSERVICESTATUS_H
#define NCI_HOST_NATIVES_HWSERVICESTATUS_H

#include <vector>
#include <string>
#include <memory>

#include "ServiceManager.h"

namespace hwhal {

class HwServiceStatus {
public:
    bool valid = false;
    std::weak_ptr<IBaseService> currentAdapter;
    int serviceProcessId = -1;
    std::string devicePath;
    std::string serviceName; // short name
    std::string serviceExecPath;

    static std::vector<HwServiceStatus> enumerateHwServices();
};

}

#endif //NCI_HOST_NATIVES_HWSERVICESTATUS_H
