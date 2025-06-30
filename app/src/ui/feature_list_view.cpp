#include "feature_list_view.hpp"

#include <imgui.h>
#include <string_view>
#include <vector>

namespace {

tree_node create_dummy_tree() {
  return {.name = "Root Feature",
          .children = {{"Feature Group 1",
                        {{"Feature 1.1", {}, "Meta info 1.1"},
                         {"Feature 1.2", {}, "Meta info 1.2"},
                         {"Feature 1.3", {}, "Meta info 1.3"}},
                        "Group meta info 1"},
                       {"Feature Group 2",
                        {{"Feature 2.1", {}, "Meta info 2.1"},
                         {"SubGroup 2.2",
                          {{"Feature 2.2.1", {}, "Meta info 2.2.1"},
                           {"Feature 2.2.2", {}, "Meta info 2.2.2"}},
                          "SubGroup meta info 2.2"}},
                        "Group meta info 2"},
                       {"Feature 3", {}, "Meta info 3"}},
          .meta_info = "Root feature meta info"};
}

} // namespace

feature_list_view::feature_list_view()
    : root(create_dummy_tree()) {
}

void feature_list_view::render_tree(const tree_node &node) {
  ImGuiTreeNodeFlags flags =
      node.children.empty() ? ImGuiTreeNodeFlags_Leaf : 0;

  bool node_open = ImGui::TreeNodeEx(node.name.data(), flags);
  if (ImGui::IsItemClicked())
    selected_node = &node;

  if (node_open) {
    for (const auto &child : node.children)
      render_tree(child);
    ImGui::TreePop();
  }
}

void feature_list_view::render() {
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar |
                                  ImGuiWindowFlags_AlwaysVerticalScrollbar;
  ImGui::Begin("Scrollable Feature List", nullptr, window_flags);

  render_tree(root);

  if (selected_node) {
    ImGui::Separator();
    ImGui::TextWrapped("Meta Information: %.*s",
                       static_cast<int>(selected_node->meta_info.size()),
                       selected_node->meta_info.data());
  }

  ImGui::End();
}