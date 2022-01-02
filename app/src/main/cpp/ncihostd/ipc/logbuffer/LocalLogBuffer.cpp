//
// Created by kinit on 2022-01-02.
//

#include "LocalLogBuffer.h"

using namespace logbuffer;

LocalLogBuffer &LocalLogBuffer::getInstance() {
    static LocalLogBuffer gwInstance;
    return gwInstance;
}

void LocalLogBuffer::append(const LogEntryRecord &entry) {
    LogEntryRecord log = entry;
    // fill in the sequence number
    log.id = mSequenceNumber++;
    std::scoped_lock lock(mMutex);
    mLogRecords.push_back(log);
    // drop the oldest log records if the buffer is full
    if (mLogRecords.size() > MAX_LOG_ENTRIES) {
        mLogRecords.pop_front();
    }
}

std::vector<LogEntryRecord> LocalLogBuffer::getAllLogs() {
    std::scoped_lock lock(mMutex);
    std::vector<LogEntryRecord> logs;
    logs.reserve(mLogRecords.size());
    for (const auto &log: mLogRecords) {
        logs.push_back(log);
    }
    return logs;
}

std::vector<LogEntryRecord> LocalLogBuffer::getLogsPartial(uint32_t start, uint32_t count) {
    std::scoped_lock lock(mMutex);
    if (mLogRecords.empty()) {
        return {};
    }
    std::vector<LogEntryRecord> logs;
    // iterate the log records
    auto it = mLogRecords.cbegin();
    while (it != mLogRecords.cend()) {
        if (auto &log = *it; log.id >= start) {
            // found the start index
            if (logs.size() >= count) {
                break;
            }
            // add the log to the result list
            logs.push_back(log);
        }
        it++;
    }
    return logs;
}

size_t LocalLogBuffer::getLogCount() const {
    return mLogRecords.size();
}

void LocalLogBuffer::clear() noexcept {
    std::scoped_lock lock(mMutex);
    mLogRecords.clear();
}

uint32_t LocalLogBuffer::getFirstLogEntryIndex() noexcept {
    std::scoped_lock lock(mMutex);
    if (mLogRecords.empty()) {
        return 0;
    } else {
        return mLogRecords.front().id;
    }
}
