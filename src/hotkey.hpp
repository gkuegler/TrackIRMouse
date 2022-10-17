#ifndef TRACKIRMOUSE_HOCKEY_HPP
#define TRACKIRMOUSE_HOCKEY_HPP

#include <windows.h>

#include "Log.hpp"

enum
{ // For applications this must be between 0 and 0xBFFF
  HOTKEY_ID_SCROLL_LAST = 5,
};

// map global windows hotkey
class GlobalHotkey
{
public:
  int profile_id_ = 0;

  GlobalHotkey(){};
  GlobalHotkey(HWND hWnd, int profile_id, UINT modifier, UINT virtual_keycode)
    : hWnd_(hWnd)
    , profile_id_(profile_id)
  {
    if (RegisterHotKey(hWnd, profile_id_, modifier, virtual_keycode)) {
      is_registered_ = true;
    }
    spdlog::debug("hotkey registered");
  }

  ~GlobalHotkey()
  {
    if (is_registered_) {
      UnregisterHotKey(hWnd_, profile_id_);
      spdlog::debug("hotkey un-registered");
    }
    spdlog::debug("hotkey destroyed");
  }

private:
  HWND hWnd_ = NULL;
  bool is_registered_ = false;
};

#endif /* TRACKIRMOUSE_HOCKEY_HPP */
