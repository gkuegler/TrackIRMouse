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

private:
  std::shared_ptr<spdlog::logger> logger_;
  std::atomic<bool> tracking_allowed_to_run_ = true;
  std::atomic<bool> pause_mouse_ = false;

public:
  TrackIR(handlers::MouseHandler* handler);
  ~TrackIR();

  void initialize(HWND hWnd,
                  bool auto_find_dll,
                  std::string user_dll_folder,
                  int profile_id);
  retcode start(); // main tracking loop
  void toggle_mouse();
  void stop();

private:
  void connect_to_np_track_ir();
  void disconnect_from_np_trackir();
};
} // namespace trackers

#endif /* TRACKIRMOUSE_TRACK_HPP */
