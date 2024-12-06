
#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

class ThreadPool {
 public:
  explicit ThreadPool(size_t);
  ~ThreadPool();
  template <class F, class... Args>
  auto Enqueue(F&& f, Args&&... args)
      -> std::future<decltype(f(args...))> {
    using return_type = decltype(f(args...));

    auto package_task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = package_task->get_future();
    do {
      std::unique_lock<std::mutex> lock(task_mutex_);
      if (stop_) return res;

      auto task = [package_task]() -> void { (*package_task)(); };
      tasks_.emplace(task);
    } while (false);
    condition_.notify_one();
    return res;
  }
  template <class F, class... Args>
  void EnqueueNoResult(F&& f, Args&&... args) {
    auto bind_func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    do {
      std::unique_lock<std::mutex> lock(task_mutex_);
      if (stop_) return;

      auto task = [bind_func]() -> void { bind_func(); };
      tasks_.emplace(task);
    } while (false);
    condition_.notify_one();
  }

  void Stop();
  void ClearTask();

 private:
  void ThreadProc();

  std::mutex task_mutex_;
  std::condition_variable condition_;
  using TaskType = std::function<void()>;
  std::queue<TaskType> tasks_;
  bool stop_;
  std::vector<std::thread> workers_;
};

// 单线程，可作为 message loop 使用
class MessageLoop : public ThreadPool {
 public:
  MessageLoop() : ThreadPool(1) {}

  template <class F, class... Args>
  void PostTask(F&& f, Args&&... args) {
    EnqueueNoResult(std::forward<F>(f), std::forward<Args>(args)...);
  }
};

#endif  // THREADPOOL_H_
