//
// Created by kinit on 2021-11-28.
//

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <dirent.h>
#include <algorithm>

#include "../log/Log.h"

#include "ProcessUtils.h"

namespace utils {

static const char *const LOG_TAG = "ProcessUtils";

std::vector<ProcessInfo> getRunningProcessInfo() {
    std::vector<ProcessInfo> runningProcesses;
    std::string procPath = "/proc/";
    DIR *dir = opendir(procPath.c_str());
    if (dir == nullptr) {
        LOGE("Failed to open %s", procPath.c_str());
        return {};
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_DIR) {
            continue;
        }
        std::string pidStr = entry->d_name;
        if (!std::all_of(pidStr.begin(), pidStr.end(), ::isdigit)) {
            continue;
        }
        int pid = std::stoi(pidStr);
        std::string procExePath = procPath + pidStr + "/exe";
        char exe[256];
        ssize_t len = readlink(procExePath.c_str(), exe, sizeof(exe) - 1);
        if (len < 0) {
            continue;
        }
        exe[len] = '\0';
        // get process uid
        int uid = -1;
        std::string procStatusPath = procPath + pidStr + "/status";
        std::ifstream procStatusFile(procStatusPath);
        if (procStatusFile.is_open()) {
            std::string line;
            while (std::getline(procStatusFile, line)) {
                if (line.find("Uid:") == 0) {
                    std::string uidStr = line.substr(5);
                    uid = std::stoi(uidStr);
                    break;
                }
            }
            procStatusFile.close();
        } else {
            uid = -1;
        }
        ProcessInfo processInfo = {};
        processInfo.pid = pid;
        processInfo.uid = uid;
        processInfo.exe = exe;
        if (!processInfo.exe.empty()) {
            processInfo.name = processInfo.exe.substr(processInfo.exe.find_last_of('/') + 1);
        }
        runningProcesses.push_back(processInfo);
    }
    closedir(dir);
    return runningProcesses;
}

}