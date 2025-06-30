#pragma once

#include <event_bus.hpp>

#include <vector>

#include <expected>
#include <sl/sl_device.h>

struct discovered_device {
  sl_device           *device;
  sl_device_descriptor descriptor;
};

class device_manager {
public:
  device_manager(event_bus&);
  ~device_manager();

  std::expected<void, sl_error> discover_devices(uint32_t timeout_ms) noexcept;
  const std::vector<discovered_device>& get_discovered_devices() const noexcept;
  void open_device() noexcept;
private:
  event_bus& event_bus_;
  std::vector<discovered_device> discovered_devices_;
};