#pragma once

#include <memory> /* std::shared_ptr for vk objects */

#include <glfw/glfw3.h>

#include <device.hpp>
#include <gpu.hpp>
#include <instance.hpp>
#include <swapchain/surface.hpp>
#include <swapchain/swapchain.hpp>

#include "event_bus.hpp"

struct frame {
  vk::UniqueCommandPool command_pool;
  vk::CommandBuffer     cmd;
  vk::UniqueSemaphore   image_available;
  vk::UniqueSemaphore   render_finished;
  vk::UniqueFence       in_flight;
};

class application {
public:
  application();
  ~application();
  
  void run();

private:
  GLFWwindow                               *window_{nullptr};
  std::shared_ptr<engine::instance>         instance_;
  std::vector<std::shared_ptr<engine::gpu>> gpus_;
  std::shared_ptr<engine::gpu>              selected_gpu_;

  std::shared_ptr<engine::device> device_;

  std::shared_ptr<engine::queue> graphics_queue_;
  std::shared_ptr<engine::queue> compute_queue_;
  std::shared_ptr<engine::queue> transfer_queue_;

  std::shared_ptr<engine::surface>   surface_;
  vk::UniqueDescriptorPool           descriptor_pool_;
  std::shared_ptr<engine::swapchain> swapchain_;
  std::vector<vk::UniqueImageView>   swapchain_views_;
  std::vector<frame>                 frames_;
  vk::Extent2D                       swapchain_extent_;
  uint32_t                           cur_frame_ = 0;

  event_bus           event_bus_;
  subscription_handle device_open_subscription_;

#ifdef APP_USE_VULKAN_DEBUG_UTILS
  vk::UniqueDebugUtilsMessengerEXT debug_utils_messenger_;
#endif

  static void glfw_error_callback(int error, const char *description);

  void init_window();
  void init_vk();
  void create_swapchain();
  void init_imgui();
  void create_swapchain_views();
  void create_frames();
  void recreate_swapchain();
  void record(frame &f, uint32_t img_idx);

  vk::SurfaceFormatKHR
  select_surface_format(const std::vector<vk::Format> &requested_formats,
                        vk::ColorSpaceKHR              requested_color_space) {
    auto available_formats =
        selected_gpu_->handle().getSurfaceFormatsKHR(surface_->handle());
    if (available_formats.size() == 1 &&
        available_formats[0].format == vk::Format::eUndefined) {
      return {requested_formats[0], requested_color_space};
    }
    for (const auto &available_format : available_formats) {
      for (auto req_format : requested_formats) {
        if (available_format.format == req_format &&
            available_format.colorSpace == requested_color_space)
          return available_format;
      }
    }
    return available_formats[0];
  }

  vk::PresentModeKHR
  choose_present_mode(const std::vector<vk::PresentModeKHR> &modes) {
    for (auto m : modes)
      if (m == vk::PresentModeKHR::eMailbox)
        return m;

    for (auto m : modes)
      if (m == vk::PresentModeKHR::eImmediate)
        return m;

    return vk::PresentModeKHR::eFifo;
  }
};