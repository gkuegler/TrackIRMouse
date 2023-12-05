#ifndef TRACKIRMOUSE_TRACK_HPP
#define TRACKIRMOUSE_TRACK_HPP

#include "windows-wrapper.hpp"

#include <exception>
#include <string>

#include "handlers.hpp"
#include "log.hpp"
#include "types.hpp"

// using HandleFunction = void (*)(Degrees, Degrees);

namespace trackers {

class TrackIR
{
public:
  // TODO: This seems clunky to have a separate handler object in the track
  // class. It should probably be just a pointer to a function?
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
  // TODO: Convert to exceptions
  void start();        // main tracking loop
  void toggle_mouse(); // for debugging purposes
  void stop();

private:
  void connect_to_np_track_ir();
  void disconnect_from_np_trackir();
};

class error_device_not_present : public std::exception
{
public:
  error_device_not_present(const char* message)
    : std::exception(message){};
};
} // namespace trackers

#endif /* TRACKIRMOUSE_TRACK_HPP */
