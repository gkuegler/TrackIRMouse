#ifndef TRACKIRMOUSE_HOOKS_HPP
#define TRACKIRMOUSE_HOOKS_HPP

#include "handlers.hpp"
#include <windows.h>

class WindowChangedHook
{
public:
  WindowChangedHook();
  ~WindowChangedHook();

private:
  HWINEVENTHOOK h_hook_window_change = 0;
  HWINEVENTHOOK h_scroll = 0;
};

#endif /* TRACKIRMOUSE_HOOKS_HPP */
