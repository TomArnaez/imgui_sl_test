#pragma once

#include <utility/slot_map.hpp>
#include <vulkan/vulkan.hpp>

namespace engine {

struct resource_access {
  vk::PipelineStageFlags2 stage_mask = {};
  vk::AccessFlags2        access_mask = {};
  vk::ImageLayout         image_layout = {};
  uint32_t                queue_family_index = vk::QueueFamilyIgnored;

  bool contains_write() const noexcept {
    constexpr auto w =
        vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eShaderWrite |
        vk::AccessFlagBits2::eTransferWrite | vk::AccessFlagBits2::eHostWrite;
    return static_cast<bool>(access_mask & w);
  }

  bool contains_read() const noexcept {
    constexpr auto r =
        vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eShaderRead |
        vk::AccessFlagBits2::eTransferRead | vk::AccessFlagBits2::eHostRead;
    return static_cast<bool>(access_mask & r);
  }
};

struct buffer_state {
  resource_access access = {};
  vk::Buffer      buffer = nullptr;
};

struct image_state {
  resource_access access = {};
  vk::ImageLayout image_layout = vk::ImageLayout::eUndefined;
  vk::Image       image = nullptr;
};

class task_graph {
public:
    [[nodiscard]]
};

} // namespace engine