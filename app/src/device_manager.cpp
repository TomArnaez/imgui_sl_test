#include "device_manager.hpp"

device_manager::device_manager(event_bus &event_bus) : event_bus_(event_bus) {
  sl_library_open();
}

device_manager::~device_manager() { sl_library_close(); }

std::expected<void, sl_error>
device_manager::discover_devices(uint32_t discovery_timeout) noexcept {
  uint32_t device_count;
  sl_error err =
      sl_enumerate_devices(&device_count, discovery_timeout, nullptr);

  std::vector<sl_device *> devices(device_count);
  err = sl_enumerate_devices(&device_count, discovery_timeout, devices.data());

  if (err != SL_ERROR_SUCCESS)
    return std::unexpected(err);

  discovered_devices_.resize(device_count);
  for (uint32_t i = 0; i < device_count; ++i) {
    auto &discovered_device = discovered_devices_[i];
    discovered_device.device = devices[i];

    err = sl_device_descriptor_get(discovered_device.device,
                                   &discovered_device.descriptor);
    if (err != SL_ERROR_SUCCESS)
      return std::unexpected(err);
  }

  return {};
}

const std::vector<discovered_device> &
device_manager::get_discovered_devices() const noexcept {
  return discovered_devices_;
}

void device_manager::open_device() noexcept {
  event_bus_.publish(device_open_requested{});
}
