#pragma once
#ifndef TIRMOUSE_HOOKS_HPP
#define TIRMOUSE_HOOKS_HPP

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

#endif /* TIRMOUSE_HOOKS_HPP */
