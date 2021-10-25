
// Created by kinit on 2021-10-25.
//

#include <unistd.h>
#include <cinttypes>
#include <fcntl.h>
#include <cerrno>
#include <cstring>

#include "ProcessView.h"

using namespace elfsym;

std::vector<std::string> string_split(const std::string &str, const std::string &splits) {
    std::vector<std::string> res;
    if (str.empty()) {
        return res;
    }
    std::string strs = str + splits;
    size_t pos = strs.find(splits);
    size_t step = splits.size();
    while (pos != std::string::npos) {
        std::string temp = strs.substr(0, pos);
        res.push_back(temp);
        strs = strs.substr(pos + step, strs.size());
        pos = strs.find(splits);
    }
    return res;
}

int ProcessView::readProcess(int pid) {
    mPointerSize = 0;
    mProcessModules.clear();
    char buffer[256];
    snprintf(buffer, 64, "/proc/%d/exe", pid);
    int fd = open(buffer, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return errno;
    }
    if (read(fd, buffer, 6) != 6) {
        close(fd);
        return errno;
    }
    close(fd);
    if (buffer[0] == 0x7F && buffer[1] == 'E' && buffer[2] == 'L' && buffer[3] == 'F') {
        char type = buffer[4];
        if (type == 1) {
            mPointerSize = 4;
        } else if (type == 2) {
            mPointerSize = 8;
        } else {
            mPointerSize = 0;
            return EINVAL;
        }
    } else {
        mPointerSize = 0;
        return EINVAL;
    }
    snprintf(buffer, 64, "/proc/%d/maps", pid);
    // xHook Copyright (c) 2018-present, iQIYI, Inc. https://github.com/iqiyi/xHook
    FILE *pf = fopen(buffer, "r");
    if (!pf) {
        return errno;
    }
    char prot[5];
    char *path;
    int pathname_pos = 0;
    size_t pathname_len;
    uint64_t address = 0;
    uint64_t offset = 0;
    while (fgets(buffer, sizeof(buffer), pf)) {
        if (sscanf(buffer, "%" PRIx64 "-%*lx %4s %" PRIx64 " %*x:%*x %*ld%n",
                   &address, prot, &offset, &pathname_pos) != 3) {
            continue;
        }
        // don't check 'r': Android 10 maps system library .text as executive only memory
        if (prot[3] != 'p') {
            // do not touch the shared memory
            continue;
        }
        // check offset
        // We are trying to find ELF header in memory.
        // It can only be found at the beginning of a mapped memory regions
        // whose offset is 0.
        if (offset != 0) {
            continue;
        }
        // get pathname
        while (isspace(buffer[pathname_pos]) && pathname_pos < (int) (sizeof(buffer) - 1)) {
            pathname_pos += 1;
        }
        if (pathname_pos >= (int) (sizeof(buffer) - 1)) {
            continue;
        }
        path = buffer + pathname_pos;
        pathname_len = strlen(path);
        if (pathname_len == 0) {
            continue;
        }
        if (path[pathname_len - 1] == '\n') {
            path[pathname_len - 1] = '\0';
            pathname_len -= 1;
        }
        if (pathname_len == 0 || path[0] == '[') {
            continue;
        }
        auto tmp = string_split(path, "/");
        mProcessModules.emplace_back(Module{tmp[tmp.size() - 1], path, address});
    }
    fclose(pf);
    return 0;
}

int ProcessView::getPointerSize() const noexcept {
    return mPointerSize;
}

bool ProcessView::isValid() const noexcept {
    return mPointerSize != 0;
}

std::vector<ProcessView::Module> ProcessView::getModules() const {
    return mProcessModules;
}
