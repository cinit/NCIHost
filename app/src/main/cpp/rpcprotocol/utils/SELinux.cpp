//
// Created by kinit on 2021-11-16.
//
#include <unistd.h>
#include <fcntl.h>
#include <sys/xattr.h>
#include <cerrno>

#include "SELinux.h"

bool SELinux::isEnabled() {
    return access("/sys/fs/selinux", F_OK) == 0 || errno == EACCES;
}

bool SELinux::isEnforcing() {
    int fd = open("/sys/fs/selinux/enforce", O_RDONLY);
    if (fd < 0) {
        return errno == EACCES;
    } else {
        char result = 0;
        if (read(fd, &result, 1) > 0) {
            close(fd);
            return result == '1';
        }
        int err = errno;
        close(fd);
        return err == EACCES;
    }
}

int SELinux::getFileSEContext(int fd, std::string *context) {
    char buf[128] = {};
    if (fgetxattr(fd, "security.selinux", buf, 128) < 0) {
        return -errno;
    } else {
        *context = buf;
        return 0;
    }
}

int SELinux::getFileSEContext(const char *path, std::string *context) {
    char buf[128] = {};
    if (getxattr(path, "security.selinux", buf, 128) < 0) {
        return -errno;
    } else {
        *context = buf;
        return 0;
    }
}

int SELinux::setFileSEContext(int fd, std::string_view context) {
    if (fsetxattr(fd, "security.selinux", context.data(), context.size() + 1, 0) < 0) {
        if (errno == ENOTSUP) {
            char buf[128] = {};
            if ((fgetxattr(fd, "security.selinux", buf, 128) == 0) && (context == buf)) {
                return 0;
            } else {
                return -ENOTSUP;
            }
        } else {
            return -errno;
        }
    } else {
        return 0;
    }
}

int SELinux::setFileSEContext(const char *path, std::string_view context) {
    if (setxattr(path, "security.selinux", context.data(), context.size() + 1, 0) < 0) {
        if (errno == ENOTSUP) {
            char buf[128] = {};
            if ((getxattr(path, "security.selinux", buf, 128) == 0) && (context == buf)) {
                return 0;
            } else {
                return -ENOTSUP;
            }
        } else {
            return -errno;
        }
    } else {
        return 0;
    }
}

int SELinux::getProcessSecurityContext(int pid, std::string *context) {
    std::string attrPath = "/proc/" + std::to_string(pid) + "/attr/current";
    int fd = open(attrPath.c_str(), O_RDONLY | O_CLOEXEC);
    char buf[128] = {};
    if (fd < 0) {
        return -errno;
    } else {
        if (read(fd, buf, 128) > 0) {
            *context = buf;
            close(fd);
            return 0;
        } else {
            int err = errno;
            close(fd);
            return -err;
        }
    }
}
