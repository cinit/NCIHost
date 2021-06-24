#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <vector>
#include <queue>

class ThreadPool {
public:
    using Task = std::function<void()>;

    explicit ThreadPool(int num);

    ~ThreadPool();

    void start();

    void shutdown();

    void execute(const Task &task);

private:
    void run();

public:
    // disable copy and assign construct
    ThreadPool(const ThreadPool &) = delete;

    ThreadPool &operator=(const ThreadPool &other) = delete;

private:
    std::atomic_bool _is_running = false; // thread pool manager status
    std::mutex _mtx;
    std::condition_variable _cond;
    int _thread_num;
    std::vector<std::thread> _threads;
    std::queue<Task> _tasks;
};

#endif // !_THREAD_POOL_H_
