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

inline std::string format_ip_address(uint32_t ip) {
  return std::format("{}.{}.{}.{}", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
                     (ip >> 8) & 0xFF, ip & 0xFF);
}

} // namespace util
