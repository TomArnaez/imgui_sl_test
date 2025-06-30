#include <ranges> //  std::views, std::ranges::to

#include <vulkan/vulkan.hpp>

#include <gpu.hpp>

namespace engine {

instance::instance(std::span<const vk::ValidationFeatureEnableEXT> en_features,
                   std::span<vk::ValidationFeatureDisableEXT>      dis_features,
                   std::span<const char *const> enabled_extensions,
                   std::span<const char *const> enabled_layers,
                   vk::InstanceCreateFlags      create_flags,
                   const vk::ApplicationInfo   &application_info)
    : flags_(create_flags) {

  vk::StructureChain<vk::InstanceCreateInfo, vk::ValidationFeaturesEXT>
      creation_chain;

  creation_chain.get<vk::InstanceCreateInfo>()
      .setFlags(create_flags)
      .setPApplicationInfo(&application_info)
      .setPEnabledExtensionNames(enabled_extensions)
      .setPEnabledLayerNames(enabled_layers);

  creation_chain.get<vk::ValidationFeaturesEXT>()
      .setEnabledValidationFeatures(en_features)
      .setDisabledValidationFeatures(dis_features);

  handle_ =
      vk::createInstanceUnique(creation_chain.get<vk::InstanceCreateInfo>());
}

vk::Instance instance::handle() const noexcept { return *handle_; }

std::vector<std::shared_ptr<gpu>>
instance::enumerate_gpus(std::shared_ptr<instance> &instance) {
  return instance->handle_.get().enumeratePhysicalDevices() |
         std::views::transform([&](vk::PhysicalDevice pd) {
           return std::make_shared<gpu>(pd, instance);
         }) |
         std::ranges::to<std::vector<std::shared_ptr<gpu>>>();
}

} // namespace engine