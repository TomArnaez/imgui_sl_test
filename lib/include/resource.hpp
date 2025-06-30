#pragma once

#include <vulkan/vulkan.hpp>

#include <image.hpp>
#include <swapchain.hpp>
#include <utility/mutex_protected.hpp>
#include <utility/slot_map.hpp>

namespace engine {

struct buffer_access {
  vk::PipelineStageFlags pipeline_stages;
  vk::AccessFlags2       access_flags;
  uint32_t               queue_family_index;
};

struct image_access {
  vk::PipelineStageFlags pipeline_stages;
  vk::AccessFlags2       access_flags;
  vk::ImageLayout        image_layout;
  uint32_t               queue_family_index;
};

struct buffer_state {
  vk::Buffer                     buffer;
  mutex_protected<buffer_access> last_access;
};

struct image_state {
  std::shared_ptr<image>        image;
  mutex_protected<image_access> last_access;
};

struct swapchain_semaphore_state {
  vk::Semaphore image_available_semaphore;
  vk::Semaphore pre_present_completed_semaphore;
};

struct frame_in_flight {
  vk::Fence             fence;
  std::atomic<uint64_t> current_frame;
};

struct swapchain_state {
  std::shared_ptr<swapchain>  swapcain;
  std::vector<swapchain_semaphore_state> semaphores;
  std::vector<frame_in_flight>           frames_in_flight;
};

class resource_manager {
public:
private:
  vk::Device                device_;
  slot_map<vk::Buffer>      buffers_;
  slot_map<vk::Image>       images_;
  slot_map<swapchain_state> swapchains_;
};

} // namespace engine