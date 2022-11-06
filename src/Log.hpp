#ifndef TRACKIRMOUSE_LOG_H
#define TRACKIRMOUSE_LOG_H

#define SPDLOG_NO_THREAD_ID
#define SPDLOG_NO_ATOMIC_LEVELS // code never modifies a logger_'s log levels
                                // concurrently by different threads.

#include <spdlog/spdlog.h> // used to export default logger

#include <memory>
#include <string>

namespace mylogging {

void
SetUpLogging();

std::shared_ptr<spdlog::logger>
GetClonedLogger(std::string name);

} // namespace mylogging

#endif /* TRACKIRMOUSE_LOG_H */
