#pragma once

#include <variant>

struct device_open_requested {
};

using event = std::variant<
    device_open_requested>;