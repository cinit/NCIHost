//
// Created by kinit on 2022-01-02.
//

#ifndef NCI_HOST_NATIVES_LOGENTRYRECORD_H
#define NCI_HOST_NATIVES_LOGENTRYRECORD_H

#include <cstdint>
#include <vector>
#include <string>
#include <string_view>

#include "Log.h"

class LogEntryRecord {
public:
    uint32_t id;
    uint64_t timestamp;
    Log::Level level;
    std::string tag;
    std::string message;

    LogEntryRecord() : id(0), timestamp(0), level(Log::Level::UNKNOWN) {}

    LogEntryRecord(uint32_t id, uint64_t timestamp, Log::Level level, std::string_view tag, std::string_view msg) :
            id(id), timestamp(timestamp), level(level), tag(tag), message(msg) {}

    [[nodiscard]] bool deserializeFromByteVector(const std::vector<uint8_t> &src);

    [[nodiscard]] std::vector<uint8_t> serializeToByteVector() const;

};

#endif //NCI_HOST_NATIVES_LOGENTRYRECORD_H
