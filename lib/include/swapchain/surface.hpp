#pragma once

#include <instance.hpp>

#include <glfw/glfw3.h>
#include <vulkan/vulkan.hpp>

namespace engine {

class surface {
public:
  surface(GLFWwindow *window, std::shared_ptr<instance>& instance);

  ~surface() noexcept;

  vk::SurfaceKHR handle() const noexcept;

private:
  vk::SurfaceKHR            handle_;
  std::shared_ptr<instance> instance_;
};

} // namespace engine::swapchain