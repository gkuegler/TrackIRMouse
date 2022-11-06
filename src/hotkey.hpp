#ifndef TRACKIRMOUSE_HOTKEY_HPP
#define TRACKIRMOUSE_HOTKEY_HPP

#include <windows.h>

#include "log.hpp"

enum
{ // Windows: For applications this must be between 0 and 0xBFFF
  HOTKEY_ID_SCROLL_ALTERNATE = 5,
};

/**
 * map a global windows hotkey
 */
class GlobalHotkey
{
public:
  int hk_id = 0;

  GlobalHotkey(){};
  GlobalHotkey(HWND hWnd, int hk_id, UINT modifier, UINT virtual_keycode)
    : hWnd_(hWnd)
    , hk_id(hk_id)
  {
    if (RegisterHotKey(hWnd, hk_id, modifier, virtual_keycode)) {
      is_registered_ = true;
    }
  }

  ~GlobalHotkey()
  {
    if (is_registered_) {
      UnregisterHotKey(hWnd_, hk_id);
    }
  }

private:
  HWND hWnd_ = NULL;
  bool is_registered_ = false;
};

#endif /* TRACKIRMOUSE_HOTKEY_HPP */
