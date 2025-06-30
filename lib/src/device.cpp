#include <device.hpp>
#include <queue.hpp>

namespace engine {

vk::Device device::handle() const noexcept { return handle_.get(); }

const std::shared_ptr<gpu> &device::physical_device() const noexcept {
  return physical_device_;
}

device_bundle device::create(std::shared_ptr<gpu> &gpu, const feature_chain &fc,
                             std::span<char const *const> wanted_exts) {
  auto dev = std::shared_ptr<device>(new device(gpu, fc, wanted_exts));

  const auto &dci = fc.get<vk::DeviceCreateInfo>();

  std::vector<std::shared_ptr<engine::queue>> queues;
  queues.reserve(8);

  for (auto i : std::views::iota(0u, dci.queueCreateInfoCount)) {
    const auto &qci = dci.pQueueCreateInfos[i];

    for (auto q : std::views::iota(0u, qci.queueCount)) {
      auto qi = vk::DeviceQueueInfo2()
                    .setFlags(qci.flags)
                    .setQueueFamilyIndex(qci.queueFamilyIndex)
                    .setQueueIndex(q);
      queues.emplace_back(std::make_shared<queue>(dev, qi));
    }
  }

  return device_bundle{.dev = std::move(dev), .queues = std::move(queues)};
}

device::device(std::shared_ptr<gpu> gpu, const feature_chain &fc,
               std::span<char const *const> exts)
    : physical_device_(std::move(gpu)), extensions_(exts.begin(), exts.end()),
      features_(fc) {
  handle_ = physical_device_->handle().createDeviceUnique(
      fc.get<vk::DeviceCreateInfo>());
}

} // namespace engine