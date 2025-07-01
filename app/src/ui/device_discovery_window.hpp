#pragma once

#include <optional>
#include <string>

#include <device_manager.hpp>

class device_discovery_window {
public:
  device_discovery_window(device_manager &);

  void render();

private:
  device_manager device_manager_;

  uint32_t                discovery_timeout_ms_ = 200;
  std::string             discovery_error_message_;
  std::optional<uint32_t> selected_device_idx_ = std::nullopt;
};