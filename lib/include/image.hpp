#pragma once

#include <vulkan/vulkan.hpp>

namespace engine {

struct image {
  vk::Image       image;
  vk::ImageType   type;
  vk::Format      image_format;
  vk::Extent3D    extent;
  uint32_t        mip_layers;
  uint32_t        array_layers;
  vk::SharingMode sharing_mode;
  vk::ImageLayout initial_layout;
};

} // namespace engine
