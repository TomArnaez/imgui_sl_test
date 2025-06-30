#pragma once

#include <memory> //  std::shared_ptr
#include <span>   //  std::span
#include <vector> //  std::vector

#include <vulkan/vulkan.hpp>

namespace engine {

class gpu;

struct instance {
  instance(std::span<const vk::ValidationFeatureEnableEXT> en_features,
           std::span<vk::ValidationFeatureDisableEXT>      dis_features,
           std::span<const char *const>                    enabled_extensions,
           std::span<const char *const>                    enabled_layers,
           vk::InstanceCreateFlags                         create_flags,
           const vk::ApplicationInfo                      &application_info);

  static std::vector<std::shared_ptr<gpu>>
  enumerate_gpus(std::shared_ptr<instance> &instance);

  vk::Instance handle() const noexcept;
private:
  vk::UniqueInstance      handle_;
  vk::InstanceCreateFlags flags_;
};

} // namespace engine