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
#include <wx/log.h>
#include <wx/wx.h>

#include <mutex>

#include "types.hpp"
// TODO: high dpi support, and text size
// TODO: test dpi support on 4K monito

void SendThreadMessage(msgcode code, wxString msg, long optional_param = 0) {
  wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD);
  event->SetInt(static_cast<int>(code));
  event->SetString(msg);
  event->SetExtraLong(static_cast<long>(optional_param));
  wxTheApp->QueueEvent(event);
}

void SendThreadMessage(msgcode code, wxString msg) {
  wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD);
  event->SetString(msg);
  event->SetInt(static_cast<long>(code));
  wxTheApp->QueueEvent(event);
}

namespace mylogging {

template <typename Mutex>
class WxSink : public spdlog::sinks::base_sink<Mutex> {
 protected:
  void sink_it_(const spdlog::details::log_msg &msg) override {
    // log_msg is a struct containing the log entry info like level, timestamp,
    // thread id etc. msg.raw contains pre formatted log

    // Format message according to sink specific formatter.
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    wxString message(fmt::to_string(formatted));

    // raise a wxWidgets error box oand/or shut down app if fatal
    if (msg.level == spdlog::level::critical) {
      wxLogFatalError(message);
    } else if (msg.level == spdlog::level::err) {
      wxLogError(message);
    } else {
      // send output to log consol in app
      if (msg.level > spdlog::level::info) {
        SendThreadMessage(msgcode::log_red_text, message, 0);
      } else {
        SendThreadMessage(msgcode::log_normal_text, message, 0);
      }
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

/**
 * Get new logger with standard sinks, named name.
 *
 * @param[in]  {string} name - Name of logger.
 *
 * @return shared pointer to logger
 */
std::shared_ptr<spdlog::logger> MakeLoggerFromStd(std::string name){

  // cutom logger with sink specific logging level
  // to wx txt control on app main panel
  static std::shared_ptr<WxSink_mt> wx_txtctrl_sink = std::make_shared<WxSink_mt>();
  wx_txtctrl_sink->set_level(spdlog::level::info);

  // create sinks (3) in total
  // Loggers accessing the same files need to use the same sinks or "weird things
  // happen". Create static sinks to be reuses
  static std::vector<spdlog::sink_ptr> sinks = {
    std::make_shared<spdlog::sinks::stdout_sink_mt>(),
    std::make_shared<spdlog::sinks::basic_file_sink_mt>("log-trackir.txt", true),
    wx_txtctrl_sink
  };

  auto logger = std::make_shared<spdlog::logger>(name, begin(sinks), end(sinks));
  logger->set_level(spdlog::level::trace);
  logger->set_pattern("[%T][%n][%l] %v"); // "[HH:MM:SS][logger_name][level] msg"

  // set sink specific format pattern after becuase setting pattern on logger
  // overrides the sink specific format pattern
  wx_txtctrl_sink->set_pattern("%v"); // "msg"
  spdlog::flush_every(std::chrono::seconds(2));

  return logger;
}

/**
 * @brief      Sets the global logger.
 *
 * @param[in]  l     The logger to be set as global and consumed.
 */
void SetGlobalLogger(std::shared_ptr<spdlog::logger> l) {
  // NOTE: setting as default also registers it in the logger registry by name
  spdlog::set_default_logger(std::move(l));
  spdlog::flush_every(std::chrono::seconds(2));
}
// clang-format on
}  // namespace mylogging
