#pragma once

#include <fmt/format.h>
#include <source_location>
#include <spdlog/spdlog.h>

namespace util {

template <typename... Args>
[[noreturn]]
void log_and_throw(
    fmt::format_string<Args...> fmt, Args &&...args,
    const std::source_location  loc = std::source_location::current()) {
  auto msg = fmt::format(fmt, std::forward<Args>(args)...);

  spdlog::error("{}:{} [{}] {}", loc.file_name(), loc.line(),
                loc.function_name(), msg);

  throw std::runtime_error(std::move(msg));
}

} // namespace util
