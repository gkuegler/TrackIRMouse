#ifndef TRACKIRMOUSE_LOG_H
#define TRACKIRMOUSE_LOG_H

#define SPDLOG_NO_THREAD_ID
#define SPDLOG_NO_ATOMIC_LEVELS  // code never modifies a logger's log levels
                                 // concurrently by different threads.

#include <spdlog/spdlog.h>  // used to export default logger

namespace mylogging {
void SetUpLogging();
}

#endif /* TRACKIRMOUSE_LOG_H */
