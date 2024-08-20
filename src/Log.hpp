#pragma once
#ifndef TIRMOUSE_LOG_H
#define TIRMOUSE_LOG_H

#define SPDLOG_NO_THREAD_ID
#define SPDLOG_NO_ATOMIC_LEVELS // code never modifies a logger_'s log levels
                                // concurrently by different threads.

// used to export logging mechanisms to other files
#include "spdlog/spdlog.h"

#include <array>
#include <map>
#include <memory>
#include <string>

namespace logging {

inline const auto log_levels =
  std::array<std::string, 7>{ "trace", "debug",    "info", "warn",
                              "error", "critical", "off" };

inline const std::map<std::string, spdlog::level::level_enum> map_name_to_level{
  { "trace", spdlog::level::trace }, { "debug", spdlog::level::debug },
  { "info", spdlog::level::info },   { "warn", spdlog::level::warn },
  { "error", spdlog::level::err },   { "critical", spdlog::level::critical },
  { "off", spdlog::level::off }
};
inline const std::map<spdlog::level::level_enum, std::string> map_level_to_name{
  { spdlog::level::trace, "trace" }, { spdlog::level::debug, "debug" },
  { spdlog::level::info, "info" },   { spdlog::level::warn, "warn" },
  { spdlog::level::err, "error" },   { spdlog::level::critical, "critical" },
  { spdlog::level::off, "off" }
};

void
SetUpLogging();

std::shared_ptr<spdlog::logger>
GetClonedLogger(std::string name);

} // namespace mylogging

#endif /* TIRMOUSE_LOG_H */
