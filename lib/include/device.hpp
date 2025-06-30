#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <gpu.hpp>

namespace engine {

using feature_chain =
    vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan12Features,
                       vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceVulkan14Features>;

class queue;
class device;

struct device_bundle {
  std::shared_ptr<device>             dev;
  std::vector<std::shared_ptr<queue>> queues;
};

class device {
public:
  vk::Device handle() const noexcept;
  const std::shared_ptr<gpu>& physical_device() const noexcept;

  static device_bundle create(std::shared_ptr<gpu>        &gpu,
                              const feature_chain         &fc,
                              std::span<char const *const> wanted_exts = {});

private:
  device(std::shared_ptr<gpu> gpu, const feature_chain &fc,
         std::span<char const *const> exts = {});

  vk::UniqueDevice         handle_;
  std::shared_ptr<gpu>     physical_device_;
  std::vector<std::string> extensions_;
  feature_chain            features_;
};
} // namespace engine