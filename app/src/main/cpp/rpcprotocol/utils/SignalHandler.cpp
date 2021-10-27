//
// Created by kinit on 2021-10-27.
//

#include <unistd.h>
#include <cstring>

#include "SignalHandler.h"

SignalHandler *SignalHandler::sInstance = nullptr;

SignalHandler::SignalHandler() = default;

SignalHandler::~SignalHandler() = default;

SignalHandler &SignalHandler::getInstance() {
    if (sInstance == nullptr) {
        // sInstance will NEVER be released
        sInstance = new SignalHandler();
    }
    return *sInstance;
}

int SignalHandler::attachCurrentThread(uint64_t signalBits, jmp_buf *jmpBuf) {
    {
        for (int sig: std::vector<int>{SIGSEGV, SIGFPE, SIGILL, SIGBUS, SIGSYS, SIGPIPE})
            if (((1u << sig) & signalBits) != 0) {
                struct sigaction newSigAct = {};
                struct sigaction oldSigAct = {};
                newSigAct.sa_sigaction = &SignalHandler::onSignalActionCallback;
                newSigAct.sa_flags = SA_SIGINFO;
                sigaction(sig, &newSigAct, &oldSigAct);
            }
    }
    int tid = gettid();
    std::scoped_lock lk(mLock);
    mThreadSignalInfo.putIfAbsent(tid);
    auto *sigStack = mThreadSignalInfo.get(tid);
    sigStack->emplace_back(ThreadSignalInfo{jmpBuf, signalBits});
    return int(sigStack->size());
}

void SignalHandler::detachCurrentThread(int depth) noexcept {
    if (depth < 0) {
        return;
    }
    int tid = gettid();
    std::scoped_lock lk(mLock);
    auto *sigStack = mThreadSignalInfo.get(tid);
    if (sigStack != nullptr) {
        while (sigStack->size() >= depth) {
            sigStack->erase(sigStack->begin() + ssize_t(sigStack->size() - 1));
        }
        if (sigStack->empty()) {
            mThreadSignalInfo.remove(tid);
        }
    }
}

bool SignalHandler::getCurrentThreadSignalInfo(SignalHandler::SignalInfo *info, int depth) noexcept {
    if (depth <= 0) {
        return false;
    }
    int tid = gettid();
    std::scoped_lock lk(mLock);
    const auto *pSigStack = mThreadSignalInfo.get(tid);
    if (pSigStack != nullptr && !pSigStack->empty() && depth <= pSigStack->size()) {
        const auto &sigStack = *pSigStack;
        const auto &sigInfo = sigStack[depth - 1];
        if (sigInfo.siginfo.si_signo != 0) {
            info->signo = sigInfo.currentSignal;
            info->ucontext = sigInfo.ucontext;
            info->siginfo = sigInfo.siginfo;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void SignalHandler::onSignalActionCallback(int sig, siginfo_t *siginfo, void *uctx) noexcept {
    int tid = gettid();
    auto *pSigStack = SignalHandler::getInstance().mThreadSignalInfo.get(tid);
    if (pSigStack != nullptr && !pSigStack->empty()) {
        auto &sigStack = *pSigStack;
        // signal is al
        for (int i = int(sigStack.size()) - 1; i >= 0; i--) {
            auto &info = sigStack[i];
            if ((1u << sig) & info.signalMask) {
                info.currentSignal = sig;
                info.siginfo = *siginfo;
                info.ucontext = *((ucontext_t *) uctx);
                siglongjmp(*info.jmpBuf, 1);
            }
        }
    }
}
