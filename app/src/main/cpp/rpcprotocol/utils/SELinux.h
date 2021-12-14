//
// Created by kinit on 2021-11-16.
//

#ifndef NCI_HOST_NATIVES_SELINUX_H
#define NCI_HOST_NATIVES_SELINUX_H

#include <string>
#include <string_view>

class SELinux {
public:
    /**
     * Check if SELinux is enabled.
     * @return true if SELinux is enabled, false otherwise.
     */
    static bool isEnabled();

    /**
     * Check if SELinux is enabled and enforcing.
     * @return true if SELinux is enabled and enforcing, false otherwise.
     */
    static bool isEnforcing();

    /**
     * Get the security context of the file descriptor.
     * @param fd the file descriptor.
     * @param context the security context.
     * @return 0 on success, -errno on error.
     */
    static int getFileSEContext(int fd, std::string *context);

    /**
     * Get the security context of the file.
     * @param path the file path.
     * @param context the security context.
     * @return 0 on success, -errno on error.
     */
    static int getFileSEContext(const char *path, std::string *context);

    /**
     * Set the security context of the file.
     * @param fd the file descriptor.
     * @param context the security context.
     * @return 0 on success, -errno on error.
     */
    static int setFileSEContext(int fd, std::string_view context);

    /**
     * Set the security context of the file.
     * @param path the file path.
     * @param context the security context.
     * @return 0 on success, -errno on error.
     */
    static int setFileSEContext(const char *path, std::string_view context);

    /**
     * Get the security context of the process.
     * @param pid the process id.
     * @param context the security context.
     * @return 0 on success, -errno on error.
     */
    static int getProcessSecurityContext(int pid, std::string *context);
};

#endif //NCI_HOST_NATIVES_SELINUX_H
