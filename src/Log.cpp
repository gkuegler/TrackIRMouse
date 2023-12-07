/**
 * Logging interface for program.
 *
 * --License Boilerplate Placeholder--
 */

#include "log.hpp"

#include "spdlog/details/null_mutex.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/spdlog.h"
#include <wx/log.h>
#include <wx/wx.h>

#include <mutex>

#include "constants.hpp"
#include "messages.hpp"

// TODO: high dpi support, and text size
// TODO: test dpi support on 4K monitor

namespace logging {

template<typename Mutex>
class WxSink : public spdlog::sinks::base_sink<Mutex>
{
protected:
  void sink_it_(const spdlog::details::log_msg& msg) override
  {
    // log_msg is a struct containing the log entry info like level, timestamp,
    // thread p_profile_id_ etc. msg.raw contains pre formatted log

    // Format message according to sink specific formatter.
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    std::string message(fmt::to_string(formatted));

    // raise a wxWidgets error box oand/or shut down app if fatal
    SendThreadMessage(msgcode::log, message, static_cast<long>(msg.level));
  }

  // Nothing to manually flush. Message queue handles everything.
  void flush_() override { return; }
};

// Standard implementation from wiki:
// https://github.com/gabime/spdlog/wiki/4.-Sinks
using WxSink_mt = WxSink<std::mutex>;
using WxSink_st = WxSink<spdlog::details::null_mutex>;

// clang-format off

//std::shared_ptr<>

void SetUpLogging() {
	// cutom logger_ with sink sp{ecific logging level
  // to wx txt control on app main panel_
  std::shared_ptr<WxSink_mt> wx_txtctrl_sink = std::make_shared<WxSink_mt>();
  wx_txtctrl_sink->set_level(spdlog::level::info);

  // create sinks (3) in total
  // Loggers accessing the same files need to use the same sinks or "weird things
  // happen". Create static sinks to be reused.
  std::vector<spdlog::sink_ptr> sinks = {
    std::make_shared<spdlog::sinks::stdout_sink_mt>(),
    std::make_shared<spdlog::sinks::basic_file_sink_mt>(LOG_FILE_NAME, true),
    wx_txtctrl_sink
  };

  auto logger_ = std::make_shared<spdlog::logger>("main", begin(sinks), end(sinks));
  logger_->set_level(spdlog::level::trace);
  logger_->set_pattern("[%T][%n][%l] %v"); // "[HH:MM:SS][logger_name][level] msg"

  // set sink specific format pattern after becuase setting global pattern on logger_
  // overrides the sink specific format pattern
  wx_txtctrl_sink->set_pattern("%v"); // "msg"

	spdlog::set_default_logger(std::move(logger_));
  spdlog::flush_every(std::chrono::seconds(2));

  return;

}

std::shared_ptr<spdlog::logger> GetClonedLogger(std::string name) {
	return spdlog::get("main")->clone(name);
}

// clang-format on
} // namespace mylogging
