#include "win-monitor-info.hpp"

#include <algorithm>
#include <vector>

#include "log.hpp"
#include "types.hpp"
#include "utility.hpp"
#include "windows-wrapper.hpp"

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
    throw std::runtime_error(std::format("Couldn't get display info for display #: {}", count));
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
WinMonitorInfo::compare(const RectPixels& a, const RectPixels& b)
{
  if (a.left < b.left) {
    return true;
  } else {
    return false;
  }
}

WinMonitorInfo::WinMonitorInfo()
{
  // Use a callback to go through each monitor.
  if (0 == EnumDisplayMonitors(NULL, NULL, MonitorProc, (LPARAM)&rectangles)) {
    throw std::runtime_error("failed to enumerate displays");
  }

  if (rectangles.size() == 0) {
    throw std::runtime_error("0 displays enumerated. Something went wrong with Windows.");
  }

  // Ensure left most monitor is first in the list.
  // Windows does not guarantee the order of monitors returned by
  // 'EnumDisplayMonitors' and the order does change between calls.
  std::sort(rectangles.begin(), rectangles.end(), compare);

  // Find the top left-most point of the virtual desktop.
  auto x = rectangles[0].left; // left
  auto y = rectangles[0].top;  // top

  for (const auto& d : rectangles) {
    x = std::min(d.left, x);
    y = std::min(d.top, y);
  }

  top_left_point = { x, y };

  spdlog::debug("Virtual Origin Offset Horizontal: {:>5d}", x);
  spdlog::debug("Virtual Origin Offset Vertical:   {:>5d}", y);

  desktop_width = static_cast<Pixels>(GetSystemMetrics(SM_CXVIRTUALSCREEN));
  desktop_height = static_cast<Pixels>(GetSystemMetrics(SM_CYVIRTUALSCREEN));

  short_to_pixels_ratio_x = USHORT_MAX_VAL / static_cast<double>(desktop_width);
  short_to_pixels_ratio_y = USHORT_MAX_VAL / static_cast<double>(desktop_height);

  spdlog::debug("Width of Virtual Desktop:  {:>5}", desktop_width);
  spdlog::debug("Height of Virtual Desktop: {:>5}", desktop_height);
}

std::vector<RectPixels>
WinMonitorInfo::Normalize() const
{
  std::vector<RectPixels> rect(rectangles);

  for (auto& r : rect) {
    r.left -= top_left_point.x;
    r.right -= top_left_point.x;
    r.top -= top_left_point.y;
    r.bottom -= top_left_point.y;
  }

  return rect;
}
