// SPDX-License-Identifier: MIT
//
// Created by kinit on 2021-10-30.
//

#include "Log.h"

#include "DefaultSessionLogImpl.h"

DefaultSessionLogImpl::DefaultSessionLogImpl(const std::string_view &tag) :
        mLogTag(tag) {
}

void DefaultSessionLogImpl::setLogTag(const std::string_view &tag) {
    mLogTag = tag;
}

void DefaultSessionLogImpl::error(std::string_view msg) {
    Log::logBuffer(Log::Level::ERROR, mLogTag.c_str(), std::string(msg).c_str());
}

void DefaultSessionLogImpl::warn(std::string_view msg) {
    Log::logBuffer(Log::Level::WARN, mLogTag.c_str(), std::string(msg).c_str());
}

void DefaultSessionLogImpl::info(std::string_view msg) {
    Log::logBuffer(Log::Level::INFO, mLogTag.c_str(), std::string(msg).c_str());
}

void DefaultSessionLogImpl::verbose(std::string_view msg) {
    Log::logBuffer(Log::Level::VERBOSE, mLogTag.c_str(), std::string(msg).c_str());
}
