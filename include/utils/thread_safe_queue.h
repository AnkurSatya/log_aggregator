#pragma once
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

template <typename T> class ThreadSafeQueue {
public:
  void push(T item) {
    {
      std::lock_guard lock(mutex_);
      queue_.push(std::move(item));
    }
    // Notifies the caller of pop()
    cv_.notify_one();
  }

  T pop() {
    // Unique lock can release and acquire mutex as and when necessary unlike
    // lock_guard. condition_variable::wait needs to release and acquire
    // repeatedly, hence we can't use lock_guard here.
    std::unique_lock lock(mutex_);
    cv_.wait(&lock, [this] { return !queue_.empty(); });
    // queue pop does not return the value so the value should be read or moved
    // before popping.
    T item = std::move(queue_.front());
    queue_.pop();
    return item;
  }

  std::optional<T> try_pop() {
    std::unique_lock lock(mutex_);
    if (empty())
      return std::nullopt;
    T item = std::move(queue_.front());
    queue_.pop();
    return item;
  }

  bool empty() const {
    std::lock_guard lock(mutex_);
    return queue_.empty();
  }

private:
  std::queue<T> queue_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
};
