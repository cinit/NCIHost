//
// Created by kinit on 2021-10-05.
//

#ifndef RPCPROTOCOL_LOG_H
#define RPCPROTOCOL_LOG_H

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <malloc.h>

class Log {
public:
    enum class Level {
        VERBOSE = 2,
        DEBUG = 3,
        INFO = 4,
        WARN = 5,
        ERROR = 6
    };
    using LogHandler = void (*)(Level level, const char *tag, const char *msg);
private:
    static volatile LogHandler mHandler;
public:
    static void format(Level level, const char *tag, const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 3, 4))) {
        va_list valist;
        LogHandler h = mHandler;
        if (h == nullptr) {
            return;
        }
        size_t size = strlen(fmt) * 3 / 2 + 16;
        void *buffer = malloc(size);
        if (buffer == nullptr) {
            return;
        }
        va_start(valist, fmt);
        vsnprintf(reinterpret_cast<char *> (buffer), size, fmt, valist);
        va_end(valist);
        h(level, tag, static_cast<const char *>(buffer));
        free(buffer);
    }

    static inline LogHandler getLogHandler() noexcept {
        return mHandler;
    }

    static inline void setLogHandler(LogHandler h) noexcept {
        mHandler = h;
    }
};

#define LOGE(...)  Log::format(Log::Level::ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...)  Log::format(Log::Level::WARN, LOG_TAG, __VA_ARGS__)
#define LOGI(...)  Log::format(Log::Level::INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...)  Log::format(Log::Level::DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGV(...)  Log::format(Log::Level::VERBOSE, LOG_TAG, __VA_ARGS__)

#endif //RPCPROTOCOL_LOG_H
