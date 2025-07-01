#include "device_discovery_window.hpp"

#include <algorithm>
#include <imgui.h>
#include <imgui_stdlib.h>

#include <util.hpp>

device_discovery_window::device_discovery_window(device_manager &device_manager)
    : device_manager_(device_manager) {}

void device_discovery_window::render() {
  if (ImGui::Begin("Device Discovery")) {
    if (ImGui::Button("Discover Devices")) {
      discovery_error_message_.clear();
      selected_device_idx_ = std::nullopt;

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
      ImGui::TextColored(error_color, "Discovery Error: %s",
                         discovery_error_message_.c_str());
      ImGui::Separator();
    }

    if (ImGui::BeginTable("Devices", 2,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_SizingFixedFit)) {
      ImGui::TableSetupColumn("Interface");
      ImGui::TableSetupColumn("ID");
      ImGui::TableHeadersRow();

      const auto &discovered_devices = device_manager_.get_discovered_devices();
      for (int i = 0; i < discovered_devices.size(); ++i) {
        const auto &discovered_device = discovered_devices[i];

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        const char *interface_text = "Unknown";
        std::string id_text = "Unknown";

        switch (discovered_device.descriptor.device_interface) {
        case SL_DEVICE_INTERFACE_GEV:
          interface_text = "GigE";
          id_text = util::format_ip_address(
              discovered_device.descriptor.gev_descriptor.device_ip_address);
          break;
        }

        const bool is_selected = (i == *selected_device_idx_);
        if (ImGui::Selectable(interface_text, is_selected,
                              ImGuiSelectableFlags_SpanAllColumns)) {
          selected_device_idx_ = i;
        }

        ImGui::TableNextColumn();
        ImGui::TextUnformatted(id_text.c_str());
      }
      ImGui::EndTable();
    }

    ImGui::Separator();

    if (selected_device_idx_) {
      if (ImGui::Button("Connect")) {
        const auto &devices = device_manager_.get_discovered_devices();
        if (*selected_device_idx_ < devices.size()) {
          const auto &selected_device = devices[*selected_device_idx_];
        }
      }
    }
  }

  ImGui::End();
}