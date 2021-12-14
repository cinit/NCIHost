//
// Created by kinit on 2021-12-10.
//
#include "protocol/ArgList.h"
#include "utils/SharedBuffer.h"

#include "INciHostDaemon.h"

using namespace std;
using namespace ipcprotocol;

bool INciHostDaemon::HistoryIoOperationEventList::deserializeFromByteVector(const vector<uint8_t> &src) {
    if (src.empty()) {
        return false;
    }
    ArgList args(src.data(), src.size());
    size_t count = 0;
    if (!(args.get(&totalStartIndex, 0) && args.get(&totalCount, 1)
          && args.get(&count, 2))) {
        return false;
    }
    // pull out the events
    events.resize(count);
    for (size_t i = 0; i < count; i++) {
        if (!args.get(&events[i], int(i + 3))) {
            return false;
        }
    }
    // pull out the event payloads
    payloads.resize(count);
    for (size_t i = 0; i < count; i++) {
        if (!args.get(&payloads[i], int(i + count + 3))) {
            return false;
        }
    }
    return true;
}

std::vector<uint8_t> INciHostDaemon::HistoryIoOperationEventList::serializeToByteVector() const {
    ArgList::Builder builder;
    builder.push(totalStartIndex);
    builder.push(totalCount);
    builder.push(events.size());
    for (const auto &event: events) {
        builder.push(event);
    }
    for (const auto &payload: payloads) {
        builder.push(payload);
    }
    return builder.build().toVector();
}

bool INciHostDaemon::DaemonStatus::deserializeFromByteVector(const vector<uint8_t> &src) {
    if (src.empty()) {
        return false;
    }
    ArgList args(src.data(), src.size());
    if (!(args.get(&processId, 0) && args.get(&versionName, 1)
          && args.get(&abiArch, 2) && args.get(&daemonProcessSecurityContext, 3)
          && args.get(&isHalServiceAttached, 4) && args.get(&halServicePid, 5)
          && args.get(&halServiceUid, 6) && args.get(&halServiceExePath, 7)
          && args.get(&halServiceArch, 8) && args.get(&halServiceProcessSecurityContext, 9)
          && args.get(&halServiceExecutableSecurityLabel, 10))) {
        return false;
    }
    return true;
}

std::vector<uint8_t> INciHostDaemon::DaemonStatus::serializeToByteVector() const {
    ArgList::Builder builder;
    builder.push(processId);
    builder.push(versionName);
    builder.push(abiArch);
    builder.push(daemonProcessSecurityContext);
    builder.push(isHalServiceAttached);
    builder.push(halServicePid);
    builder.push(halServiceUid);
    builder.push(halServiceExePath);
    builder.push(halServiceArch);
    builder.push(halServiceProcessSecurityContext);
    builder.push(halServiceExecutableSecurityLabel);
    return builder.build().toVector();
}
