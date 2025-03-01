#pragma once

#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <utility>
#include <type_traits>
#include <memory>

class Thread {
    bool _destroying = false;
    std::thread _worker;
    std::queue<std::function<void()>> _jobQueue;
    std::mutex _queueMutex;
    std::condition_variable _condition;

    void queueLoop() noexcept;

public:
    Thread();
    ~Thread();

    void addJob(std::function<void()>&& function);
    void wait();
};

class ThreadPool {
    std::vector<Thread> _threads;

public:
    ThreadPool(size_t count);
    Thread& getThread(size_t index);
    size_t getNumThreads() const;
    void wait();
};