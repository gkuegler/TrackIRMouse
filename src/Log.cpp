/**
 * Logging interface for program.
 *
 * --License Boilerplate Placeholder--
 */

#include "log.hpp"

#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <wx/wx.h>

#include <mutex>

// GrepWin strings used to change to spdlog
// regex match string: LogToWix\(fmt::format\(([^ \)] *)\)\);
// replacement format string: spdlog::info\(\1);

namespace MyLogging {

template <typename Mutex>
class WxSink : public spdlog::sinks::base_sink<Mutex> {
 protected:
  void sink_it_(const spdlog::details::log_msg &msg) override {
    // log_msg is a struct containing the log entry info like level, timestamp,
    // thread id etc. msg.raw contains pre formatted log

    // Format message according to sink specific formatter.
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);

    // raise a wxWidgets error box oand/or shut down app if fatal
    if (msg.level == spdlog::level::critical) {
      wxLogFatalError(wxString(fmt::to_string(formatted)));
    } else if (msg.level == spdlog::level::err) {
      wxLogError(wxString(fmt::to_string(formatted)));
    } else {
      // send output to log consol in app
      wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD);
      event->SetString(fmt::to_string(formatted));
      if (msg.level > spdlog::level::info) {
        // used by txt control event handler to turn
        // warning messages and above red.
        event->SetExtraLong(1);
      }
      wxTheApp->QueueEvent(event);
    }
  }

  // Nothing to manually flush. Message queue handles everything.
  void flush_() override { return; }
};

// Standard implementation from wiki:
// https://github.com/gabime/spdlog/wiki/4.-Sinks
using WxSink_mt = WxSink<std::mutex>;
using WxSink_st = WxSink<spdlog::details::null_mutex>;

// clang-format off
void SetUpLogging() {
  // Creating multithreaded loggers with multiple sinks
  std::vector<spdlog::sink_ptr> sinks;

  // cutom logger with sink specific logging level
  // to wx txt control on app main panel
  auto wx_txtctrl_sink = std::make_shared<WxSink_mt>();
  wx_txtctrl_sink->set_level(spdlog::level::info);

  // create sinks (3) in total
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
  sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("log-trackir.txt", true));
  sinks.push_back(wx_txtctrl_sink);

  auto combined_logger =std::make_shared<spdlog::logger>("main", begin(sinks), end(sinks));
  combined_logger->set_level(spdlog::level::trace);
  combined_logger->set_pattern("[%T][%n][%l] %v"); // "[HH:MM:SS][logger_name][level] msg"

  // set sink specific format pattern after becuase setting pattern on logger
  // overrides the sink specific format pattern
  wx_txtctrl_sink->set_pattern("%v"); // "msg"

  // NOTE: setting as default also registers it in the logger registry by name
  spdlog::set_default_logger(std::move(combined_logger));
  spdlog::flush_every(std::chrono::seconds(2));

  return;
}
// clang-format on
}  // namespace MyLogging
