#ifndef TRACKIRMOUSE_TRACK_HPP
#define TRACKIRMOUSE_TRACK_HPP

#include <windows.h>

#include <string>

#include "handlers.hpp"
#include "log.hpp"
#include "types.hpp"

using HandleFunction = void (*)(Degrees, Degrees);

namespace trackers {
class TrackIR {
 public:
  handlers::MouseHandler* m_handler;
  // HandleFunction m_fpHandler;
  std::atomic<bool> m_tracking_allowed_to_run = false;
  std::atomic<bool> m_pause_tracking = false;

 private:
  std::shared_ptr<spdlog::logger> m_logger;
  HWND m_hWnd;
  int m_profile_id;
  std::string m_dllpath;

 public:
  TrackIR(HWND hWnd, std::string dllpath, int profile_id,
          handlers::MouseHandler* handler);
  ~TrackIR();
  retcode start();  // main tracking loop
  void toggle();
  void stop();

 private:
  void connect_to_np_track_ir();
  void disconnect_from_np_trackir();
};
}  // namespace trackers

#endif /* TRACKIRMOUSE_TRACK_HPP */
