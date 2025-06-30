#include "device_discovery_window.hpp"

#include <algorithm>
#include <imgui.h>
#include <nfd.h>

device_discovery_window::device_discovery_window(device_manager& device_manager)
  : device_manager_(device_manager) {}

void device_discovery_window::render() {
  if (ImGui::Begin("Device Discovery")) {
    if (ImGui::Button("Discover Devices")) {
      discovery_error_message_.clear();

      auto result = device_manager_.discover_devices(discovery_timeout_ms_);
      if (!result)
        discovery_error_message_ = sl_error_to_string(result.error());
    }

    ImGui::SameLine();

    ImGui::PushItemWidth(120.0f);
    if (ImGui::InputScalar("Discovery Timeout (ms)", ImGuiDataType_U32,
                           &discovery_timeout_ms_))
      discovery_timeout_ms_ = std::clamp(discovery_timeout_ms_, 50u, 1000u);

    ImGui::PopItemWidth();

    ImGui::Separator();

    if (!discovery_error_message_.empty()) {
      ImVec4 error_color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
      ImGui::TextColored(error_color, "Error: %s",
                         discovery_error_message_.c_str());
      ImGui::Separator();
    }

    if (ImGui::BeginTable("Devices", 3,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
      ImGui::TableSetupColumn("Interface");
      ImGui::TableHeadersRow();

      ImGui::EndTable();
    }
  }

  ImGui::End();
}