#include "environment.hpp"

#include <algorithm>
#include <vector>

#include "log.hpp"
#include "types.hpp"
#include "utility.hpp"
#include "windows-wrapper.hpp"

// SendInput with absolute mouse movement flag takes a short int
constexpr static double USHORT_MAX_VAL = 65535;

// windows api callback
BOOL
MonitorProc(HMONITOR hMonitor,  // handle to the display monitor.
            HDC hdc,            // handle to the device context.
            LPRECT lprcMonitor, // pointer to a RECT structure in
                                // virtual screen coordinates.
            LPARAM lParam)      // value passed from 'EnumDisplayMonitors'
{
  auto rectangles = reinterpret_cast<std::vector<RectPixels>*>(lParam);

  // Use the static count to know which monitor
  // failed if logging errors.
  static int count{ 0 };

  MONITORINFOEX info;
  info.cbSize = sizeof(MONITORINFOEX);

  if (!GetMonitorInfo(hMonitor, &info)) {
    throw std::runtime_error(
      std::format("Couldn't get display info for display #: {}", count));
  }

  // Monitor Pixel Bounds in the Virtual Desktop
  // static_cast: long -> signed int
  RectPixels r = { static_cast<Pixels>(info.rcMonitor.left),
                   static_cast<Pixels>(info.rcMonitor.right),
                   static_cast<Pixels>(info.rcMonitor.top),
                   static_cast<Pixels>(info.rcMonitor.bottom) };
  rectangles->push_back(r);
  count++;
  return true;
}

/* Function for std::compare.
 * Returns ​true if the first argument is less than (i.e. is ordered before)
 * the second.
 * https://en.cppreference.com/w/cpp/algorithm/sort.html
 */
bool
compare(const RectPixels& a, const RectPixels& b)
{
  if (a[LEFT_EDGE] < b[LEFT_EDGE]) {
    return true;
  } else {
    return false;
  }
}

WinDisplayInfo
GetHardwareDisplayInformation(bool sort_windows)
{

  // g_displays.clear();

  // Container for the WindowsCallbackProc to fill.
  std::vector<RectPixels> rectangles;

  // Use a callback to go through each monitor.
  if (0 == EnumDisplayMonitors(NULL, NULL, MonitorProc, (LPARAM)&rectangles)) {
    throw std::runtime_error("failed to enumerate displays");
  }

  if (rectangles.size() == 0) {
    throw std::runtime_error("0 displays enumerated");
  }

  if (sort_windows) {
    std::sort(rectangles.begin(), rectangles.end(), compare);
  }

  // in the event that the top-left most point may be above or below 0, 0
  // TODO: make an actual display class to represent hardware info?
  // TODO: just use the microsoft profided 'RECT', it has members 'left'
  // etc...?
  Pixels origin_offset_x_ = rectangles[0][0]; // left
  Pixels origin_offset_y_ = rectangles[0][2]; // top

  for (const auto& d : rectangles) {
    origin_offset_x_ = std::min(d[0], origin_offset_x_);
    origin_offset_y_ = std::min(d[2], origin_offset_y_);
  }

  spdlog::debug("Virtual Origin Offset Horizontal: {:>5d}", origin_offset_x_);
  spdlog::debug("Virtual Origin Offset Vertical:   {:>5d}", origin_offset_y_);

  const auto virtual_desktop_width = static_cast<Pixels>(GetSystemMetrics(
    SM_CXVIRTUALSCREEN)); // width_ of total bounds of all screens
  const auto virtual_desktop_height = static_cast<Pixels>(GetSystemMetrics(
    SM_CYVIRTUALSCREEN)); // height_ of total bounds of all screens

  spdlog::debug("Width of Virtual Desktop:  {:>5}", virtual_desktop_width);
  spdlog::debug("Height of Virtual Desktop: {:>5}", virtual_desktop_height);

  return {
    GetSystemMetrics(SM_CMONITORS),
    rectangles,
    origin_offset_x_,
    origin_offset_y_,
    USHORT_MAX_VAL / static_cast<double>(virtual_desktop_width),
    USHORT_MAX_VAL / static_cast<double>(virtual_desktop_height),
    GetSystemMetrics(SM_CXVIRTUALSCREEN),
    GetSystemMetrics(SM_CYVIRTUALSCREEN),
  };
}
