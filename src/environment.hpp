#ifndef TRACKIRMOUSE_WINDISPLAY_HPP
#define TRACKIRMOUSE_WINDISPLAY_HPP

#include "types.hpp"

namespace environment {

typedef struct win_display_info_
{
  int count = 0;
  std::vector<RectPixels> rectangles;
  Pixels origin_offset_x = 0;
  Pixels origin_offset_y = 0;
  double short_to_pixels_ratio_x = 0; // short value to desktop pixels ratio
  double short_to_pixels_ratio_y = 0; // short value to desktop pixels ratio
  int desktop_width;
  int desktop_height;
} WinDisplayInfo;

WinDisplayInfo
GetHardwareDisplayInformation();
} // namespace environment

#endif /* TRACKIRMOUSE_WINDISPLAY_HPP */
