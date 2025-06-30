#include <swapchain/surface.hpp>

namespace engine {

surface::surface(GLFWwindow *window, std::shared_ptr<instance>& instance)
    : instance_(instance) {
  if (glfwCreateWindowSurface(instance->handle(), window, nullptr,
                              reinterpret_cast<VkSurfaceKHR *>(&handle_)) !=
      VK_SUCCESS)
    throw std::runtime_error("glfwCreateWindowSurface failed");
}

surface::~surface() noexcept {
  if (handle_) {
    instance_->handle().destroySurfaceKHR(handle_, nullptr);
    handle_ = nullptr;
  }
}

vk::SurfaceKHR surface::handle() const noexcept { return handle_; }

} // namespace engine