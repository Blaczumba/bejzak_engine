#pragma once

#include <array>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "lib/types/util.h"

namespace lib::thread {

template <size_t N, typename... Args>
class Worker {
public:
  using Context = std::tuple<Args...>;
// TODO: Change after std::move_only_function becomes a standard.
#ifdef __cpp_lib_move_only_function
  using Job = std::move_only_function<void(Args...)>;
#else
  using Job = std::function<void(Args...)>;
#endif  // __cpp_lib_move_only_function

  Worker() = default;

  Worker(Args... args);

  void startWorkingThread(Args... args);

  ~Worker();

  void addJob(Job&& job);

private:
  void workingThread();

  Context _context;
  std::queue<Job> _tasks;
  std::thread _thread;
  std::mutex _mtx;
  std::condition_variable _cv;
  bool _stop;
};

template <size_t N, typename... Args>
Worker<N, Args...>::Worker(Args... args)
  : _context(std::forward<Args>(args)...), _stop(false), _thread(&Worker::workingThread, this) {}

template <size_t N, typename... Args>
Worker<N, Args...>::~Worker() {
  {
    std::lock_guard<std::mutex> lock(_mtx);
    _stop = true;
    _cv.notify_one();
  }

  if (_thread.joinable()) {
    _thread.join();
  }
}

template <size_t N, typename... Args>
void Worker<N, Args...>::startWorkingThread(Args... args) {
  if (_thread.joinable()) [[unlikely]] {
    return;
  }

  _context = Context{std::forward<Args>(args)...};
  _stop = false;
  _thread = std::thread(&Worker::workingThread, this);
}

template <size_t N, typename... Args>
void Worker<N, Args...>::workingThread() {
  std::array<Job, N> tasksToProcess;
  typename lib::SmallestIndex<N>::type i;
  while (true) {
    {
      std::unique_lock<std::mutex> lock(_mtx);
      _cv.wait_for(lock, std::chrono::seconds(5), [this]() {
        return _tasks.size() > N || _stop;
      });

      if (_stop) [[unlikely]] {
        break;
      }

      for (i = 0; i < std::min(N, _tasks.size()); i++ ) {
        tasksToProcess[i] = std::move(_tasks.front());
        _tasks.pop();
      }
    }

    for (typename lib::SmallestIndex<N>::type j = 0; j < i; j++) {
      std::apply(tasksToProcess[j], _context);
    }
  }

  while (!_tasks.empty()) {
    std::apply(_tasks.front(), _context);
    _tasks.pop();
  }
}

template <size_t N, typename... Args>
void Worker<N, Args...>::addJob(Job&& job) {
  {
    std::lock_guard<std::mutex> lock(_mtx);
    _tasks.push(std::move(job));
    if (_tasks.size() < N) {
      return;
    }
  }
  _cv.notify_one();
}

}  // namespace lib::thread
