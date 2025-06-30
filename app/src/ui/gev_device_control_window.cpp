#include "gev_device_control_window.hpp"
#include <charconv>
#include <iostream>
#include <nfd.h>

namespace {

constexpr ImVec4 COLOR_SUCCESS{0.0f, 1.0f, 1.0f, 1.0f}; // Cyan for info
constexpr ImVec4 COLOR_ERROR{1.0f, 0.0f, 0.0f, 1.0f};   // Red
constexpr ImVec4 COLOR_INFO{1.0f, 1.0f, 1.0f, 1.0f};    // White
constexpr ImVec4 COLOR_OK{0.0f, 1.0f, 0.0f, 1.0f};      // Green

} // namespace

gev_device_control_window::gev_device_control_window(std::string_view name)
    : name_(name) {}

void gev_device_control_window::render() {
  if (ImGui::Begin(name_.c_str())) {
    ImGui::Text("Register Control");
    ImGui::Separator();

    ImGui::InputText("Address (hex/dec)", address_buffer_.data(),
                     address_buffer_.size());

    ImGui::InputText("Value (hex/dec)", value_buffer_.data(),
                     value_buffer_.size());

    ImGui::Spacing();

    if (ImGui::Button("Read"))
      handle_read_register_button();
    ImGui::SameLine();
    if (ImGui::Button("Write"))
      handle_write_register_button();

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::Text("GenICam");
    ImGui::Spacing();
    if (ImGui::Button("Set XML"))
      handle_set_xml_button();
    
    ImGui::SameLine();
    if (ImGui::Button("Save XML"))
      handle_save_xml_button();

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::Text("Acquisition");
    ImGui::Spacing();
    if (ImGui::Button("Start Acquisition"))
      handle_start_acq_button();
    ImGui::SameLine();
    if (ImGui::Button("Stop Acquisition"))
      handle_stop_acq_button();

    ImGui::Spacing();
    if (ImGui::Button("Trigger"))
      handle_trigger_button();

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::Text("Status:");
    ImGui::SameLine();
    ImGui::TextColored(status_color_, "%s", status_message_.c_str());
  }
  ImGui::End();
}

std::optional<uint64_t>
gev_device_control_window::parse_input(std::string_view input_str) const {
  auto sv = std::string_view(input_str.data());

  if (sv.empty())
    return std::nullopt;

  uint64_t value = 0;
  int      base = 10;

  // Check for "0x" or "0X" prefix to automatically select base 16
  if (sv.starts_with("0x") || sv.starts_with("0X")) {
    sv.remove_prefix(2);
    base = 16;
  }

  auto [ptr, ec] =
      std::from_chars(sv.data(), sv.data() + sv.size(), value, base);

  if (ec == std::errc{} && ptr == sv.data() + sv.size()) {
    return value;
  }

  return std::nullopt;
}

void gev_device_control_window::handle_set_xml_button() {
  nfdchar_t      *out_path = nullptr;
  nfdfilteritem_t filterItem[1] = {{"XML file", "xml"}};
  nfdresult_t     result = NFD_OpenDialog(&out_path, filterItem, 1, NULL);

  if (result == NFD_OKAY)
    NFD_FreePath(out_path);
  else if (result == NFD_ERROR)
    std::cerr << "Error: " << NFD_GetError() << std::endl;
}

void gev_device_control_window::handle_save_xml_button() {
}

void gev_device_control_window::handle_write_register_button() {
  auto parsed_addr = parse_input(address_buffer_.data());
  if (!parsed_addr) {
    set_status("Error: Invalid address format.", COLOR_ERROR);
    return;
  }
  if (*parsed_addr > std::numeric_limits<uint32_t>::max()) {
    set_status("Error: Address exceeds 32-bit limit.", COLOR_ERROR);
    return;
  }

  auto parsed_val = parse_input(value_buffer_.data());
  if (!parsed_val) {
    set_status("Error: Invalid value format.", COLOR_ERROR);
    return;
  }
  if (*parsed_val > std::numeric_limits<uint32_t>::max()) {
    set_status("Error: Value exceeds 32-bit limit.", COLOR_ERROR);
    return;
  }

  const uint32_t address = static_cast<uint32_t>(*parsed_addr);
  const uint32_t value = static_cast<uint32_t>(*parsed_val);
}

void gev_device_control_window::handle_read_register_button() {
  auto parsed_addr = parse_input(address_buffer_.data());
  if (!parsed_addr) {
    set_status("Error: Invalid address format.", COLOR_ERROR);
    return;
  }

  if (*parsed_addr > std::numeric_limits<uint32_t>::max()) {
    set_status("Error: Address exceeds 32-bit limit.", COLOR_ERROR);
    return;
  }
}

void gev_device_control_window::handle_start_acq_button() {
  set_status("Acquisition started (dummy)", COLOR_OK);
}

void gev_device_control_window::handle_stop_acq_button() {
  set_status("Acquisition stopped (dummy)", COLOR_OK);
}

void gev_device_control_window::handle_trigger_button() {
  set_status("Software trigger fired (dummy)", COLOR_OK);
}

void gev_device_control_window::set_status(std::string_view message,
                                         const ImVec4    &color) {
  status_message_ = message;
  status_color_ = color;
}