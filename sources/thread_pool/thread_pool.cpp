#include "thread_pool.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

Thread::Thread() : _worker(std::thread(&Thread::queueLoop, this)) {

}

Thread::~Thread() {
    if (_worker.joinable()) {
        wait();
        {
            std::lock_guard<std::mutex> lck(_queueMutex);
            _destroying = true;
            _condition.notify_one();
        }
        _worker.join();
    }
}

void Thread::queueLoop() noexcept {
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            _condition.wait(lock, [this] { return !_jobQueue.empty() || _destroying; });
            if (_destroying) {
                break;
            }
            job = std::move(_jobQueue.front());
        }

        job();

        {
            std::lock_guard<std::mutex> lock(_queueMutex);
            _jobQueue.pop();
            if(_jobQueue.empty())
                _condition.notify_one();
        }
    }
}

void Thread::addJob(std::function<void()>&& function) {
    std::lock_guard<std::mutex> lock(_queueMutex);
    _jobQueue.emplace(std::move(function));
    _condition.notify_one();
}

void Thread::wait() {
    std::unique_lock<std::mutex> lock(_queueMutex);
    _condition.wait(lock, [this]() { return _jobQueue.empty(); });
}

ThreadPool::ThreadPool(size_t count) : _threads(count) {

}

size_t ThreadPool::getNumThreads() const {
    return _threads.size();
}

Thread& ThreadPool::getThread(size_t index) {
    return _threads[index];
}

void ThreadPool::wait() {
    for (auto& thread : _threads) {
        thread.wait();
    }
}