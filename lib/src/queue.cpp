#include <queue.hpp>

namespace engine {

queue::queue(std::shared_ptr<device>     dev,
             const vk::DeviceQueueInfo2 &queue_info)
    : device_(dev), queue_create_flags_(queue_info.flags),
      queue_family_index_(queue_info.queueFamilyIndex),
      queue_index_(queue_info.queueIndex) {
  handle_ = device_->handle().getQueue2(queue_info);
}

const vk::QueueFamilyProperties &
queue::queue_family_properties() const noexcept {
  return device_->physical_device()
      ->queue_family_properties[queue_family_index_];
}

} // namespace engine