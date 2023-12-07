#ifndef TRACKIRMOUSE_HOOKS_HPP
#define TRACKIRMOUSE_HOOKS_HPP

#include "handlers.hpp"

#include "windows-wrapper.hpp"

class HookWindowChanged
{
public:
  HookWindowChanged();
  ~HookWindowChanged();
  void Disable();

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
