//
// Created by kinit on 2021-11-28.
//

#ifndef NCI_HOST_NATIVES_PROCESSUTILS_H
#define NCI_HOST_NATIVES_PROCESSUTILS_H

#include <string>
#include <vector>

namespace utils {

struct ProcessInfo {
    int pid;
    int uid;
    std::string name;
    std::string exe;
};

// get all running process info
std::vector<ProcessInfo> getRunningProcessInfo();

}

#endif //NCI_HOST_NATIVES_PROCESSUTILS_H
