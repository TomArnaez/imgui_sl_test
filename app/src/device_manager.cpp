#include "device_manager.hpp"

std::expected<device_session, sl_error>
device_session::create(sl_device *device, uint32_t num_framebuffers,
                       uint32_t framebuffer_size) {
  sl_error          err;
  sl_device_handle *device_handle = nullptr;
  sl_stream        *stream = nullptr;

  if (err = sl_device_open(device, &device_handle)) {
    return std::unexpected(err);
  }

  std::vector<uint16_t> framebuffer_data(num_framebuffers * framebuffer_size,
                                         0);

  std::vector<sl_framebuffer_descriptor> framebuffer_descriptors;
  framebuffer_descriptors.reserve(num_framebuffers);

  for (uint32_t i = 0; i < num_framebuffers; ++i) {
    framebuffer_descriptors.push_back(sl_framebuffer_descriptor{
        .data = framebuffer_data.data() + framebuffer_size * i,
        .size = framebuffer_size});
  }

  sl_gev_stream_config stream_config = {
      .framebuffer_count = num_framebuffers,
      .framebuffer_descs = framebuffer_descriptors.data(),
      .driver_type = SL_GEV_DRIVER_TYPE_FILTER,
  };

  if (err = sl_gev_stream_create(device_handle, &stream_config, &stream)) {
    sl_device_close(device_handle);
    return std::unexpected(err);
  }

  return device_session(device_handle, stream, std::move(framebuffer_data));
}

device_session::device_session(sl_device_handle *handle, sl_stream *stream,
                               std::vector<uint16_t> &&frame_data)
    : frame_data_(std::move(frame_data)), device_handle_(handle),
      stream_(stream) {}

device_session::~device_session() {
  if (stream_)
    sl_stream_destroy(stream_);
  if (device_handle_)
    sl_device_close(device_handle_);
}

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

std::expected<void, sl_error>
device_manager::open_device(sl_device *device) noexcept {
  if (opened_device_)
    sl_device_close(opened_device_);

  sl_error err = sl_device_open(device, &opened_device_);
  if (err)
    return std::unexpected(err);
}
