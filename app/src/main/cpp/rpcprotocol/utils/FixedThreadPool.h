#ifndef FIXED_THREAD_POOL_H
#define FIXED_THREAD_POOL_H

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <vector>
#include <queue>

class FixedThreadPool {
public:
    using Task = std::function<void()>;

    explicit FixedThreadPool(int num);

    ~FixedThreadPool();

    FixedThreadPool(const FixedThreadPool &) = delete;

    FixedThreadPool &operator=(const FixedThreadPool &other) = delete;

    void start();

    void shutdown();

    void execute(const Task &task);

    [[nodiscard]] inline bool isRunning() const {
        return mIsRunning;
    }

private:
    std::atomic<bool> mIsRunning = false;
    std::mutex mLock;
    std::condition_variable mTaskLock;
    int mThreadNum;
    std::vector<std::thread> mWorkerThreads;
    std::queue<Task> mTasks;

    void run();
};

#endif // !FIXED_THREAD_POOL_H
