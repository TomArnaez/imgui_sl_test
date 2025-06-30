#pragma once

#include <chrono>   /* std::chrono::nanoseconds for timeout */
#include <expected> /* std::expected */
#include <memory>   /* std::shared_ptr for vk handles */

#include <vulkan/vulkan.hpp>

#include <device.hpp>
#include <swapchain/surface.hpp>

namespace engine {

struct acquired_image {
  uint32_t image_index;
  bool     is_suboptimal;
};

struct image_entry {
  vk::Image handle;
  bool      layout_initialised;
};

class swapchain {
public:
  swapchain(std::shared_ptr<device> device, std::shared_ptr<surface> &surface,
            vk::SwapchainCreateInfoKHR &create_info,
            vk::SurfaceFormatKHR        surface_format)
      : device_(device), surface_(surface), surface_format_(surface_format),
        min_image_count_(create_info.minImageCount) {
    handle_ = device_->handle().createSwapchainKHRUnique(create_info);

    auto vk_images = device_->handle().getSwapchainImagesKHR(*handle_);
    images_.reserve(vk_images.size());
    for (auto img : vk_images)
      images_.push_back({img, false});
  }

  swapchain(swapchain &&) = delete;
  swapchain &operator=(swapchain &&) = delete;
  swapchain(const swapchain &) = delete;
  swapchain &operator=(const swapchain &) = delete;

  [[nodiscard]]
  std::expected<acquired_image, vk::Result>
  acquire_image(vk::Semaphore semaphore, std::chrono::nanoseconds timeout) {
    try {
      auto acquire_info = vk::AcquireNextImageInfoKHR()
                              .setSemaphore(semaphore)
                              .setTimeout(timeout.count())
                              .setSwapchain(*handle_)
                              .setDeviceMask(1);

      auto rv = device_->handle().acquireNextImage2KHR(acquire_info);
      switch (rv.result) {
      case vk::Result::eSuccess:
        return acquired_image{rv.value, false};
      case vk::Result::eSuboptimalKHR:
        return acquired_image{rv.value, true};
      case vk::Result::eTimeout:
        return std::unexpected(rv.result);
      default:
        return std::unexpected(rv.result);
      }
    } catch (const vk::OutOfDateKHRError &) {
      return std::unexpected(vk::Result::eErrorOutOfDateKHR);
    } catch (const vk::SystemError &e) {
      return std::unexpected(static_cast<vk::Result>(e.code().value()));
    }
  }

  [[nodiscard]]
  std::expected<void, vk::Result> present(uint32_t      image_index,
                                          vk::Semaphore render_finished,
                                          vk::Queue     present_queue) const {
    try {
      auto present_info = vk::PresentInfoKHR()
                              .setWaitSemaphores(render_finished)
                              .setSwapchains(handle_.get())
                              .setPImageIndices(&image_index);

      auto res = present_queue.presentKHR(present_info);
      if (res == vk::Result::eErrorOutOfDateKHR ||
          res == vk::Result::eSuboptimalKHR)
        return std::unexpected(res);
      if (res != vk::Result::eSuccess)
        return std::unexpected(res);
      return {};
    } catch (const vk::OutOfDateKHRError &) {
      return std::unexpected(vk::Result::eErrorOutOfDateKHR);
    } catch (const vk::SystemError &e) {
      return std::unexpected(static_cast<vk::Result>(e.code().value()));
    }
  }

  [[nodiscard]]
  const vk::SurfaceFormatKHR &surface_format() const noexcept {
    return surface_format_;
  }

  [[nodiscard]]
  uint32_t min_image_count() const noexcept {
    return min_image_count_;
  }

  const std::vector<image_entry> &images() const noexcept { return images_; }

private:
  vk::UniqueSwapchainKHR   handle_;
  vk::SurfaceFormatKHR     surface_format_;
  uint32_t                 min_image_count_;
  std::shared_ptr<device>  device_;
  std::shared_ptr<surface> surface_;
  std::vector<image_entry> images_;
};

} // namespace engine