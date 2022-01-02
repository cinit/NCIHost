//
// Created by kinit on 2022-01-02.
//

#include "../protocol/ArgList.h"

#include "LogEntryRecord.h"

using namespace ipcprotocol;

bool LogEntryRecord::deserializeFromByteVector(const std::vector<uint8_t> &src) {
    uint32_t iLevel = 0;
    if (ArgList args(src.data(), src.size());
            args.isValid()
            && args.get(&id, 0) && args.get(&timestamp, 1) && args.get(&iLevel, 2)
            && args.get(&tag, 3) && args.get(&message, 4)) {
        level = static_cast<Log::Level>(iLevel);
        return true;
    }
    return false;
}

std::vector<uint8_t> LogEntryRecord::serializeToByteVector() const {
    auto iLevel = static_cast<uint32_t>(level);
    ArgList::Builder builder;
    builder.push(id);
    builder.push(timestamp);
    builder.push(iLevel);
    builder.push(tag);
    builder.push(message);
    return builder.build().toVector();
}
