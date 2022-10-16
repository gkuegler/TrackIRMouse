#ifndef TRACKIRMOUSE_HOCKEY_HPP
#define TRACKIRMOUSE_HOCKEY_HPP

#include <windows.h>

#include "Log.hpp"

enum {  // For applications this must be between 0 and 0xBFFF
  HOTKEY_ID_SCROLL_LAST = 5,
};

class GlobalHotkey {
  // map global windows hotkey
 public:
  int m_id = 0;
  GlobalHotkey(){};
  GlobalHotkey(HWND hWnd, int id, UINT modifier, UINT virtual_keycode)
      : m_hWnd(hWnd), m_id(id) {
    if (RegisterHotKey(hWnd, id, modifier, virtual_keycode)) {
      m_is_registered = true;
    }
    spdlog::debug("hotkey registered");
  }

  ~GlobalHotkey() {
    if (m_is_registered) {
      UnregisterHotKey(m_hWnd, m_id);
      spdlog::debug("hotkey un-registered");
    }
    spdlog::debug("hotkey destroyed");
  }

 private:
  HWND m_hWnd = NULL;
  bool m_is_registered = false;
};

#endif /* TRACKIRMOUSE_HOCKEY_HPP */
