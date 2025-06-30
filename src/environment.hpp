#pragma once

#include "types.hpp"

// TODO: Should this class be more self contained for the mouse control? Like
// should the send input function be a apart of this. Maybe this should be a
// class called 'Desktop'?
class WinDisplayInfo
{
public:
  int count = 0;
  std::vector<RectPixels> rectangles;
  Point<Pixels> top_left_point; // origin offset in pixels from (0, 0)

  // Used in normalized absolute coordinates for Win32 SendInput function.
  // Coordinate (0,0) maps onto the upper-left corner of the display surface,
  // (65535,65535) maps onto the lower-right corner.
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-mouse_event
  double short_to_pixels_ratio_x = 0;
  double short_to_pixels_ratio_y = 0;

  int desktop_width;
  int desktop_height;

  WinDisplayInfo();
  std::vector<RectPixels> Normalize() const;

private:
  static bool compare(const RectPixels& a, const RectPixels& b);
};
