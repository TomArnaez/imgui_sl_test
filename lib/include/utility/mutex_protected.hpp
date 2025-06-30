#pragma once

#include <mutex>
#include <utility>

template <typename T> class mutex_protected {
public:
  template <typename... Args>
  mutex_protected(Args &&...args) : obj_(std::forward<Args>(args)...) {}

  using lock_type = std::unique_lock<std::mutex>;

  std::pair<T &, lock_type> lock() { return {obj_, lock_type(mutex_)}; }

private:
  std::mutex mutex_;
  T          obj_;
};