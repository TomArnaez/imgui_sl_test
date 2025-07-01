#pragma once

#include <event_bus.hpp>

#include <vector>

#include <expected>
#include <sl/sl_device.h>

struct discovered_device {
  sl_device           *device;
  sl_device_descriptor descriptor;
};

class device_session {
public:
  static std::expected<device_session, sl_error>
  create(sl_device *device, uint32_t num_framebuffers,
         uint32_t framebuffer_size);

  ~device_session();

private:
  device_session(sl_device_handle *handle, sl_stream *stream,
                 std::vector<uint16_t> &&frame_data);
                 
  std::vector<uint16_t> frame_data_;
  sl_device            *device_;
  sl_device_handle     *device_handle_;
  sl_stream            *stream_;
};

class device_manager {
public:
  device_manager(event_bus &);
  ~device_manager();

  std::expected<void, sl_error> discover_devices(uint32_t timeout_ms) noexcept;
  const std::vector<discovered_device> &get_discovered_devices() const noexcept;

  std::expected<void, sl_error> open_device(sl_device *device) noexcept;

private:
  event_bus                     &event_bus_;
  std::vector<discovered_device> discovered_devices_;

  sl_device_handle *opened_device_;
};