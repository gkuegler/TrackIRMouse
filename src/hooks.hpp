#pragma once

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
