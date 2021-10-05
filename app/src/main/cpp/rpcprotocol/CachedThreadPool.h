//
// Created by kinit on 2021-10-04.
//

#ifndef RPCPROTOCOL_CACHEDTHREADPOOL_H
#define RPCPROTOCOL_CACHEDTHREADPOOL_H

#include <mutex>
#include <atomic>
#include <functional>
#include <queue>
#include <thread>
#include <memory>
#include <condition_variable>

class CachedThreadPool {
private:
    class CachedThreadPoolImpl;

public:
    using Task = std::function<void()>;

    explicit CachedThreadPool(int corePoolSize, int maxPoolSize, int idleTimeout);

    ~CachedThreadPool();

    CachedThreadPool(const CachedThreadPool &) = delete;

    CachedThreadPool &operator=(const CachedThreadPool &other) = delete;

    void start();

    void shutdown();

    void execute(const Task &task);

    [[nodiscard]] inline bool isRunning() const {
        return mIsRunning;
    }

private:
    std::atomic<bool> mIsRunning = false;
    std::mutex mStatusLock;
    std::condition_variable mTaskLock;
    int mThreadNum;
    std::vector<std::thread> mWorkerThreads;
    std::queue<Task> mTasks;

    void run();
};


#endif //RPCPROTOCOL_CACHEDTHREADPOOL_H
