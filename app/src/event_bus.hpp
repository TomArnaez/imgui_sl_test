#pragma once

#include <any>
#include <functional>
#include <typeindex>
#include <vector>

#include "events.hpp"
#include <shared_mutex>

class subscription_handle {
public:
  subscription_handle() = default;

private:
  friend class event_bus;

  subscription_handle(std::type_index type, uint64_t id)
      : event_type_index_(type), subscription_id_(id) {}

  std::type_index event_type_index_ = typeid(void);
  uint64_t        subscription_id_ = 0;

  // A non-zero ID is considered valid.
  explicit operator bool() const { return subscription_id_ != 0; }

  // For using in map keys or sets if needed, and for std::erase_if.
  friend bool operator==(const subscription_handle &,
                         const subscription_handle &) = default;
};

class event_bus {
public:
  using generic_callback = std::function<void(const std::any &)>;

  event_bus() = default;
  
  event_bus(const event_bus &) = delete;
  event_bus &operator=(const event_bus &) = delete;
  event_bus(event_bus &&) = delete;
  event_bus &operator=(event_bus &&) = delete;

  template <typename TEvent, typename TCallable>
    requires std::invocable<TCallable, const TEvent &>
  [[nodiscard]] subscription_handle subscribe(TCallable &&callback) {
    // Create a wrapper that safely casts and calls the specific callback.
    auto wrapper =
        [cb = std::forward<TCallable>(callback)](const std::any &event_any) {
          // Un-erase the type by casting the std::any to the concrete event
          // type. This is now a reference, not a pointer.
          cb(std::any_cast<const TEvent &>(event_any));
        };

    const auto     type_idx = std::type_index(typeid(TEvent));
    const uint64_t subscription_id = next_subscription_id_++;

    // Lock for exclusive access since we are modifying the map.
    std::scoped_lock lock(subscribers_mutex_);

    subscribers_[type_idx].emplace_back(
        subscription{.id = subscription_id, .callback = std::move(wrapper)});

    return {type_idx, subscription_id};
  }

  void unsubscribe(subscription_handle handle) {
    if (!handle) {
      return; // Invalid handle
    }

    std::scoped_lock lock(subscribers_mutex_);

    auto it = subscribers_.find(handle.event_type_index_);
    if (it != subscribers_.end()) {
      // Use std::erase_if to efficiently remove the subscription
      // without needing to manually find an iterator.
      std::erase_if(it->second, [&](const subscription &sub) {
        return sub.id == handle.subscription_id_;
      });
    }
  }

  template <typename TEvent> void publish(const TEvent &event) const {
    const auto type_idx = std::type_index(typeid(TEvent));

    // Use a shared lock for read-only access.
    // This allows multiple publishers to run concurrently.
    std::shared_lock lock(subscribers_mutex_);

    auto it = subscribers_.find(type_idx);
    if (it != subscribers_.end()) {
      const std::any event_any = event;
      for (const auto &sub : it->second) {
        sub.callback(event_any);
      }
    }
  }

private:
  struct subscription {
    uint64_t         id;
    generic_callback callback;
  };

  mutable std::shared_mutex subscribers_mutex_;
  std::atomic<uint64_t>     next_subscription_id_{1};
  std::unordered_map<std::type_index, std::vector<subscription>> subscribers_;
};