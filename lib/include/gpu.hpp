#pragma once

#include <ranges> /* std::ranges::to, std::views::transform */
#include <vector> /* std::vector */

#include <vulkan/vulkan.hpp>

#include <instance.hpp>

namespace engine {

struct gpu {
  vk::PhysicalDevice        handle_;
  std::shared_ptr<instance> instance_;

  vk::PhysicalDeviceProperties2          properties;
  vk::PhysicalDeviceFeatures2            features;
  vk::PhysicalDeviceMemoryProperties2    memory_properties;
  std::vector<vk::QueueFamilyProperties> queue_family_properties;
  vk::PhysicalDeviceSubgroupProperties   subgroup_properties;

  // ───────────────────────────────────────────────────────────────────────────
  gpu(vk::PhysicalDevice pd, std::shared_ptr<instance> &inst)
      : handle_{pd}, instance_(inst) {
    auto props = pd.getProperties2<vk::PhysicalDeviceProperties2,
                                   vk::PhysicalDeviceSubgroupProperties>();

    properties = props.get<vk::PhysicalDeviceProperties2>();
    subgroup_properties = props.get<vk::PhysicalDeviceSubgroupProperties>();
    features = pd.getFeatures2();
    memory_properties = pd.getMemoryProperties2();
    queue_family_properties = pd.getQueueFamilyProperties();
  }

  [[nodiscard]]
  vk::PhysicalDevice handle() const noexcept {
    return handle_;
  }
};

} // namespace engine