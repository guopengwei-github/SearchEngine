#include "threadpool.h"

void ThreadPool::ThreadProc() {
  while (true) {
    TaskType task;
    do {
      std::unique_lock<std::mutex> lock(task_mutex_);

      condition_.wait(lock, [this]() -> bool {
        return this->stop_ || !this->tasks_.empty();
      });
      if (stop_) return;
      task = std::move(tasks_.front());
      tasks_.pop();
    } while (false);
    task();
  }
}

ThreadPool::ThreadPool(size_t thread_number) : stop_(false) {
  for (size_t i = 0; i < thread_number; ++i) {
    workers_.push_back(std::thread(&ThreadPool::ThreadProc, this));
  }
}

ThreadPool::~ThreadPool() { Stop(); }

void ThreadPool::Stop() {
  std::queue<TaskType> queue_clear;
  bool stoped = false;
  do {
    std::unique_lock<std::mutex> lock(task_mutex_);
    stoped = stop_;
    if (!stop_) {
      stop_ = true;
      if (!tasks_.empty()) {
        tasks_.swap(queue_clear);
      }
    }
  } while (false);

  if (!stoped) {
    condition_.notify_all();
    for (std::thread& worker : workers_) worker.join();
  }
}

void ThreadPool::ClearTask() {
  std::queue<TaskType> queue_clear;
  std::unique_lock<std::mutex> lock(task_mutex_);
  if (!tasks_.empty()) {
    tasks_.swap(queue_clear);
  }
}
