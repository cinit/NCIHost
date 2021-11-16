//
// Created by kinit on 2021-11-16.
//

#ifndef NCI_HOST_NATIVES_SELINUX_H
#define NCI_HOST_NATIVES_SELINUX_H

#include <string>
#include <string_view>

class SELinux {
public:
    static bool isEnabled();

    static bool isEnforcing();

    static int getFileSEContext(int fd, std::string *context);

    static int getFileSEContext(const char *path, std::string *context);

    static int setFileSEContext(int fd, std::string_view context);

    static int setFileSEContext(const char *path, std::string_view context);
};

#endif //NCI_HOST_NATIVES_SELINUX_H
