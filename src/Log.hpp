#ifndef TRACKIRMOUSE_LOG_H
#define TRACKIRMOUSE_LOG_H

// format library not needed anymore with spdlog
//#define FMT_HEADER_ONLY
//#include <fmt\xchar.h>
//#include <fmt\format.h>

#define SPDLOG_NO_THREAD_ID
// code never modifies a logger's log levels concurrently by
// different threads.
#define SPDLOG_NO_ATOMIC_LEVELS
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#include <spdlog/spdlog.h> // used to export default logging

// TODO: push these out to all internal functions.
constexpr int SUCCESS = 0;
constexpr int FAILURE = -1;

// set up global logger
// have app set up logger first thing

namespace MyLogging {
  void SetUpLogging();
}

#endif /* TRACKIRMOUSE_LOG_H */
