#ifndef TRACKIRMOUSE_LOG_H
#define TRACKIRMOUSE_LOG_H

#define SPDLOG_NO_THREAD_ID
#define SPDLOG_NO_ATOMIC_LEVELS  // code never modifies a logger's log levels
                                 // concurrently by different threads.

#include <spdlog/spdlog.h>  // used to export default logger
#include <wx/wx.h>

#include <string>

#include "types.hpp"

void SendThreadMessage(msgcode code, wxString msg);

namespace mylogging {
std::shared_ptr<spdlog::logger> MakeLoggerFromStd(std::string name);
void SetGlobalLogger(std::shared_ptr<spdlog::logger> l);
void SetUpLogging(std::string);
}  // namespace mylogging

#endif /* TRACKIRMOUSE_LOG_H */
