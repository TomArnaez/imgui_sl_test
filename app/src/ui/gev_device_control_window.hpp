#pragma once

#include <imgui.h>

#include <array>
#include <optional>
#include <string>
#include <string_view>

class gev_device_control_window {
public:
  gev_device_control_window(std::string_view name = "GevDeviceControl");

  void render();

private:
  std::optional<uint64_t> parse_input(std::string_view input_str) const;

  void handle_set_xml_button();
  void handle_save_xml_button();
  void handle_read_register_button();
  void handle_write_register_button();
  void handle_start_acq_button();
  void handle_stop_acq_button();
  void handle_trigger_button();
  
  void set_status(std::string_view message, const ImVec4 &color);

  std::string name_;

  std::array<char, 256> address_buffer_{};
  std::array<char, 256> value_buffer_{};

  std::string           status_message_;
  ImVec4                status_color_{1.0f, 1.0f, 1.0f, 1.0f};
};