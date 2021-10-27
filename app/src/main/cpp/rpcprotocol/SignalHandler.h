//
// Created by kinit on 2021-10-27.
//
#ifndef NCI_HOST_NATIVES_SIGNALHANDLER_H
#define NCI_HOST_NATIVES_SIGNALHANDLER_H

#include <functional>
#include <csetjmp>
#include <csignal>
#include <cstdint>
#include <vector>
#include <mutex>

#include "HashMap.h"

/**
 * SIGSEGV, SIGFPE, SIGILL, SIGBUS, SIGSYS, SIGPIPE
 */
class SignalHandler {
public:
    using SignalInfo = struct {
        int signo;
        siginfo_t siginfo;
        ucontext_t ucontext;
    };

    template<class Result>
    class ExecutionBlock;

    SignalHandler();

    ~SignalHandler();

    SignalHandler(const SignalHandler &) = delete;

    SignalHandler &operator=(const SignalHandler &other) = delete;

    template<class R>
    [[nodiscard]] inline ExecutionBlock<R> run(const std::function<R()> &r) {
        return ExecutionBlock<R>(this, r);
    }

    [[nodiscard]]
    static SignalHandler &getInstance();

private:
    static constexpr uint64_t SIGNAL_FAULT_MASK =
            (1u << SIGSEGV) | (1u << SIGFPE) | (1u << SIGILL) | (1u << SIGBUS) | (1u << SIGSYS) | (1u << SIGPIPE);

    // return depth = 1, 2, 3...
    [[nodiscard]]
    int attachCurrentThread(uint64_t signalBits, jmp_buf *jmpBuf);

    // depth = 1, 2, 3...
    void detachCurrentThread(int depth) noexcept;

    bool getCurrentThreadSignalInfo(SignalInfo *info, int depth) noexcept;

    static uint64_t getSignalBits(const std::vector<int> &signals) noexcept {
        uint64_t result = 0;
        for (int i: signals) {
            result |= 1u << i;
        }
        return result & SIGNAL_FAULT_MASK;
    }

    template<class R>
    R runExecutionBlock(const ExecutionBlock<R> &block) {
        jmp_buf jmpBuf;
        volatile int depth = -1;
        if (sigsetjmp(jmpBuf, 1)) {
            SignalInfo signalInfo = {};
            // signal caught
            if (getCurrentThreadSignalInfo(&signalInfo, depth)) {
                detachCurrentThread(depth);
                return block.mSignalBlock(signalInfo.siginfo.si_signo, &signalInfo);
            } else {
                detachCurrentThread(depth);
                throw std::runtime_error("signal stack corrupt");
            }
        } else {
            // start execution
            depth = attachCurrentThread(block.mSignalBits, &jmpBuf, &depth);
            try {
                R &result = std::move(block.mRunBlock());
                detachCurrentThread(depth);
                return result;
            } catch (...) {
                detachCurrentThread(depth);
                throw; // return
            }
        }
    }

    template<class R= void>
    void runExecutionBlock(const ExecutionBlock<void> &block) {
        jmp_buf jmpBuf;
        volatile int depth = -1;
        if (sigsetjmp(jmpBuf, 1)) {
            // signal caught
            SignalInfo signalInfo = {};
            getCurrentThreadSignalInfo(&signalInfo, depth);
            detachCurrentThread(depth);
            return block.mSignalBlock(signalInfo.signo, &signalInfo);
        } else {
            // start execution
            depth = attachCurrentThread(block.mSignalBits, &jmpBuf);
            try {
                block.mRunBlock();
                detachCurrentThread(depth);
                return;
            } catch (...) {
                detachCurrentThread(depth);
                throw; // return
            }
        }
    }

public:
    template<class Result>
    class ExecutionBlock {
    private:
        SignalHandler *mH = nullptr;
        uint64_t mSignalBits = 0;
        std::function<Result()> mRunBlock;
        std::function<Result(int, const SignalInfo *)> mSignalBlock;
    public:
        ExecutionBlock(SignalHandler *h, const std::function<Result()> &r) :
                mH(h), mRunBlock(r) {
        }

        ~ExecutionBlock() = default;

        ExecutionBlock(const ExecutionBlock &) = delete;

        ExecutionBlock &operator=(const ExecutionBlock &other) = delete;

        [[nodiscard]] ExecutionBlock &onSignal(const std::vector<int> &signals,
                                               const std::function<Result(int, const SignalInfo *)> &handler) {
            mSignalBlock = handler;
            mSignalBits = getSignalBits(signals);
            return *this;
        }

        [[nodiscard]] inline ExecutionBlock &onSignal(int s,
                                                      const std::function<Result(int, const SignalInfo *)> &handler) {
            return onSignal(std::vector<int>{s}, handler);
        }

        inline Result commit() {
            mH->runExecutionBlock<Result>(*this);
        }

        friend SignalHandler;
    };

private:
    class ThreadSignalInfo {
    public:
        jmp_buf *jmpBuf = nullptr;
        uint64_t signalMask = 0;
        int currentSignal = 0;
        siginfo_t siginfo = {};
        ucontext_t ucontext = {};
    };

    std::mutex mLock;
    HashMap<int, std::vector<ThreadSignalInfo>> mThreadSignalInfo;

    static SignalHandler *sInstance;

    static void onSignalActionCallback(int sig, siginfo_t *siginfo, void *uctx) noexcept;
};

#endif //NCI_HOST_NATIVES_SIGNALHANDLER_H
