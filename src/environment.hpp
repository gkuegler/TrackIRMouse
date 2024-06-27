#ifndef TRACKIRMOUSE_WINDISPLAY_HPP
#define TRACKIRMOUSE_WINDISPLAY_HPP

#include "types.hpp"

struct WinDisplayInfo
{
  int count = 0;
  // TODO: use regular rectangle with '.left' member variables for access or
  // redo the math for the 'graphic'
  std::vector<RectPixels> rectangles;
  Pixels origin_offset_x = 0;
  Pixels origin_offset_y = 0;
  double short_to_pixels_ratio_x = 0; // short value to desktop pixels ratio
  double short_to_pixels_ratio_y = 0; // short value to desktop pixels ratio
  int desktop_width;
  int desktop_height;
};

WinDisplayInfo
GetHardwareDisplayInformation(bool);

#endif /* TRACKIRMOUSE_WINDISPLAY_HPP */
