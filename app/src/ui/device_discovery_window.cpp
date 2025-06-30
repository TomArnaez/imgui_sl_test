#include "device_discovery_window.hpp"

#include <algorithm>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <nfd.h>

#include <util.hpp>

device_discovery_window::device_discovery_window(device_manager &device_manager)
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
      ImGui::TableSetupColumn("ID");
      ImGui::TableHeadersRow();

      for (auto &discovered_device : device_manager_.get_discovered_devices()) {
        ImGui::TableNextRow();

        // Interface
        ImGui::TableNextColumn();
        switch (discovered_device.descriptor.device_interface) {
        case SL_DEVICE_INTERFACE_GEV:
          ImGui::Text("GigE");
          break;
        default:
          ImGui::Text("Unknown");
        }

        ImGui::TableNextColumn();
        switch (discovered_device.descriptor.device_interface) {
        case SL_DEVICE_INTERFACE_GEV: {
          std::string ip_str = util::format_ip_address(discovered_device.descriptor.gev_descriptor.device_ip_address);
          ImGui::Text(ip_str.c_str());
          break;
        }
        default:
          ImGui::Text("Unknown");
        }
      }

      ImGui::EndTable();
    }
  }

  ImGui::End();
}