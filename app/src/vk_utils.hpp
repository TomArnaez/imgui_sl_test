//----------------------------------
// gpu_helpers.hpp
//----------------------------------
#pragma once
#include <expected> // C++23
#include <optional>
#include <ranges>
#include <set>
#include <vulkan/vulkan.hpp>

namespace vk_utils {
/// True if a queue family supports graphics.
[[nodiscard]]
constexpr bool is_graphics_queue(const vk::QueueFamilyProperties &q) noexcept {
  return static_cast<bool>(q.queueFlags & vk::QueueFlagBits::eGraphics);
}

/// Return the first index of a graphics-capable queue family, or nullopt.
[[nodiscard]]
inline std::optional<std::uint32_t> first_graphics_queue(const engine::gpu &g) {
  auto indices = std::views::iota(
      0u, static_cast<std::uint32_t>(g.queue_family_properties.size()));

  auto zipped = std::views::zip(indices, g.queue_family_properties);

  auto it = std::ranges::find_if(
      zipped, [](const auto &p) { return is_graphics_queue(std::get<1>(p)); });

  if (it == zipped.end())
    return std::nullopt;
  return std::get<0>(*it);
}

/// Pick the first discrete GPU that has a graphics queue.
[[nodiscard]]
inline std::expected<std::shared_ptr<engine::gpu>, std::string>
pick_discrete_gpu(const std::vector<std::shared_ptr<engine::gpu>> &gpus) {
  auto it = std::ranges::find_if(gpus, [](const auto &g) {
    return g->properties.properties.deviceType ==
               vk::PhysicalDeviceType::eDiscreteGpu &&
           first_graphics_queue(*g).has_value();
  });

  if (it == gpus.end())
    return std::unexpected{"No suitable discrete GPU with a graphics queue"};

  return *it;
}

struct selected_families {
  uint32_t graphics = UINT32_MAX;
  uint32_t compute = UINT32_MAX;
  uint32_t transfer = UINT32_MAX;

  [[nodiscard]] bool complete() const noexcept {
    return graphics != UINT32_MAX;
  }
};

[[nodiscard]] constexpr bool supports(const vk::QueueFamilyProperties &p,
                                      vk::QueueFlags bits) noexcept {
  return static_cast<bool>(p.queueFlags & bits);
}

[[nodiscard]] constexpr bool dedicated(const vk::QueueFamilyProperties &p,
                                       vk::QueueFlags bits) noexcept {
  return supports(p, bits) && (p.queueFlags & ~bits) == vk::QueueFlags{};
}

[[nodiscard]]
inline std::expected<selected_families, std::string>
pick_queue_families(const engine::gpu &gpu) {
  const auto &props = gpu.queue_family_properties;

  auto indices = std::views::iota(0u, static_cast<uint32_t>(props.size()));
  auto zipped = std::views::zip(indices, props);

  auto choose = [&](vk::QueueFlags      caps,
                    std::set<uint32_t> &taken) -> std::optional<uint32_t> {
    auto it = std::ranges::find_if(zipped, [&](auto const &z) {
      auto [idx, prop] = z;
      return dedicated(prop, caps) && !taken.contains(idx);
    });

    if (it != zipped.end())
      return std::get<0>(*it);

    it = std::ranges::find_if(zipped, [&](auto const &z) {
      auto [idx, prop] = z;
      return supports(prop, caps) && !taken.contains(idx);
    });
    if (it != zipped.end())
      return std::get<0>(*it);

    return std::nullopt;
  };

  selected_families  out;
  std::set<uint32_t> used;

  if (auto g = choose(vk::QueueFlagBits::eGraphics, used))
    used.insert(out.graphics = *g);
  else
    return std::unexpected{"device exposes no graphics queue"};

  if (auto c = choose(vk::QueueFlagBits::eCompute, used))
    used.insert(out.compute = *c);
  else
    out.compute = out.graphics; // fall back

  if (auto t = choose(vk::QueueFlagBits::eTransfer, used))
    used.insert(out.transfer = *t);
  else
    out.transfer = (out.compute != UINT32_MAX) ? out.compute : out.graphics;

  return out;
}
} // namespace vk_utils
