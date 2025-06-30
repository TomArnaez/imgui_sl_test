#pragma once

#include <vector>
#include <string_view>

struct tree_node {
    std::string_view name;
    std::vector<tree_node> children;
    std::string_view meta_info;
};

class feature_list_view {
public:
    feature_list_view();
    void render();
private:
    void render_tree(const tree_node& node);

    tree_node root;
    const tree_node* selected_node = nullptr;
};