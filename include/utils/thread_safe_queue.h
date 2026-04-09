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

  T pop(std::stop_token token) {
    // This function would put the caller thread to sleep if the condition
    // variable predicate is not true.

    // Unique lock can release and acquire mutex
    // as and when necessary unlike lock_guard. condition_variable::wait needs
    // to release and acquire the mutex and lock_guard does not allow manual
    // releasing of lock.
    std::unique_lock lock(mutex_);
    // Wakes up when:
    // 1. queue is not empty
    // 2. Someone calls cv_.notify_one() or cv_notify_all()
    // 3. token.stop_requested() is true.
    cv_.wait(lock, token, [this] { return !queue_.empty(); });
    if (token.stop_requested() && queue_.empty())
      return T{};
    // queue pop does not return the value so the value should be read or moved
    // before popping.
    T item = std::move(queue_.front());
    queue_.pop();
    return item;
  }

  std::optional<T> try_pop() {
    // Unlike pop(), this function would return with/without an item without
    // waiting.
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
  std::condition_variable_any cv_;
};
