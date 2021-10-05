#include <thread>
#include <mutex>

#include "FixedThreadPool.h"

FixedThreadPool::FixedThreadPool(int num) : mThreadNum(num) {}

FixedThreadPool::~FixedThreadPool() {
    if (mIsRunning) {
        shutdown();
    }
}

void FixedThreadPool::start() {
    if (!mIsRunning.exchange(true)) {
        for (int i = 0; i < mThreadNum; i++) {
            mWorkerThreads.emplace_back(std::thread(&FixedThreadPool::run, this));
        }
    }
}

void FixedThreadPool::shutdown() {
    {
        // stop thread pool, should notify all threads to wake
        std::unique_lock lk(mLock);
        mIsRunning = false;
        mTaskLock.notify_all(); // must do this to avoid thread block
    }
    // terminate every thread job
    for (std::thread &t: mWorkerThreads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void FixedThreadPool::execute(const Task &task) {
    if (mIsRunning) {
        std::unique_lock lk(mLock);
        mTasks.push(task);
        mTaskLock.notify_one(); // wake a thread to do the task
    }
}

void FixedThreadPool::run() {
    // every thread will compete to pick up task from the queue to do the task
    pthread_setname_np(pthread_self(), "pool-worker");
    while (mIsRunning) {
        Task task;
        {
            std::unique_lock lk(mLock);
            if (!mTasks.empty()) {
                // if tasks not empty,
                // must finish the task whether thread pool is running or not
                task = mTasks.front();
                mTasks.pop(); // remove the task
            } else if (mIsRunning && mTasks.empty())
                mTaskLock.wait(lk);
        }
        if (task) {
            task(); // do the task
        }
    }
}
