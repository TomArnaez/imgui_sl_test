#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <memory> /* std::shared_ptr for device */

#include <device.hpp>

namespace engine {

class queue {
public:
  queue(std::shared_ptr<device> dev, const vk::DeviceQueueInfo2 &queue_info);

  vk::Queue handle() const noexcept { return handle_; }

  const vk::QueueFamilyProperties &queue_family_properties() const noexcept;
  uint32_t queue_family_index() const noexcept { return queue_family_index_; }
  uint32_t queue_index() const noexcept { return queue_index_; }

private:
  vk::Queue                  handle_;
  std::shared_ptr<device>    device_;
  vk::DeviceQueueCreateFlags queue_create_flags_;
  uint32_t                   queue_family_index_;
  uint32_t                   queue_index_; // Index within family
};

} // namespace engine