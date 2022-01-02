//
// Created by kinit on 2022-01-02.
//

#ifndef NCI_HOST_NATIVES_LOCALLOGBUFFER_H
#define NCI_HOST_NATIVES_LOCALLOGBUFFER_H

#include <cstdint>
#include <vector>
#include <deque>
#include <mutex>
#include <atomic>

#include "rpcprotocol/log/LogEntryRecord.h"

namespace logbuffer {

class LocalLogBuffer {
private:
    constexpr static size_t MAX_LOG_ENTRIES = 4096;

    LocalLogBuffer() = default;

public:
    LocalLogBuffer(const LocalLogBuffer &) = delete;

    LocalLogBuffer &operator=(const LocalLogBuffer &) = delete;

    [[nodiscard]] static LocalLogBuffer &getInstance();

    void append(const LogEntryRecord &entry);

    [[nodiscard]] std::vector<LogEntryRecord> getAllLogs();

    [[nodiscard]] std::vector<LogEntryRecord> getLogsPartial(uint32_t start, uint32_t count);

    [[nodiscard]] uint32_t getFirstLogEntryIndex() noexcept;

    [[nodiscard]] size_t getLogCount() const;

    void clear() noexcept;

private:
    std::deque<LogEntryRecord> mLogRecords;
    std::mutex mMutex;
    std::atomic_uint32_t mSequenceNumber = 1;
};

}

#endif //NCI_HOST_NATIVES_LOCALLOGBUFFER_H
