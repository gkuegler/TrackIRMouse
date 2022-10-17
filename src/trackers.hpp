#ifndef TRACKIRMOUSE_TRACK_HPP
#define TRACKIRMOUSE_TRACK_HPP

#include <windows.h>

#include <string>

#include "handlers.hpp"
#include "log.hpp"
#include "types.hpp"

// using HandleFunction = void (*)(Degrees, Degrees);

namespace trackers {
class TrackIR
{
public:
  handlers::MouseHandler* handler_;
  std::atomic<bool> tracking_allowed_to_run_ = false;
  std::atomic<bool> pause_tracking_ = false;

private:
  std::shared_ptr<spdlog::logger> logger_;

public:
  TrackIR(HWND hWnd,
          std::string dll_path,
          int profile_id,
          handlers::MouseHandler* handler);
  ~TrackIR();
  retcode start(); // main tracking loop
  void toggle();
  void stop();

private:
  void connect_to_np_track_ir();
  void disconnect_from_np_trackir();
};
} // namespace trackers

#endif /* TRACKIRMOUSE_TRACK_HPP */
