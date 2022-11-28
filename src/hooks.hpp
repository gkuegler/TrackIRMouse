#ifndef TRACKIRMOUSE_HOOKS_HPP
#define TRACKIRMOUSE_HOOKS_HPP

#include "handlers.hpp"
#include <windows.h>

class WindowChangedHook
{
public:
  WindowChangedHook();
  ~WindowChangedHook();
  auto Disable() -> void;

private:
  HWINEVENTHOOK hook = 0;
};

// class ScrollEventHook
//{
// public:
//   ScrollEventHook();
//   ~ScrollEventHook();
//
// private:
//   HWINEVENTHOOK hook = 0;
// };

#endif /* TRACKIRMOUSE_HOOKS_HPP */
