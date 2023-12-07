#include "environment.hpp"

#include "windows-wrapper.hpp"

#include "Log.hpp"

// SendInput with absolute mouse movement flag takes a short int
constexpr static double USHORT_MAX_VAL = 65535;

static std::vector<RectPixels> g_displays;

// windows api callback
BOOL
PopulateVirtMonitorBounds(HMONITOR hMonitor,
                          HDC hdcMonitor,
                          LPRECT lprcMonitor,
                          LPARAM lParam)
{
  static int count{ 0 };
  MONITORINFOEX monitor;
  monitor.cbSize = sizeof(MONITORINFOEX);
  if (!GetMonitorInfo(hMonitor, &monitor)) {
    throw std::runtime_error(
      std::format("Couldn't get display info for display #: {}", count));
  }

  // Monitor Pixel Bounds in the Virtual Desktop
  // static_cast: long -> signed int
  RectPixels r = { static_cast<Pixels>(monitor.rcMonitor.left),
                   static_cast<Pixels>(monitor.rcMonitor.right),
                   static_cast<Pixels>(monitor.rcMonitor.top),
                   static_cast<Pixels>(monitor.rcMonitor.bottom) };
  g_displays.push_back(r);
  count++;
  return true;
}

WinDisplayInfo
GetHardwareDisplayInformation()
{
  spdlog::trace("entering GetHardwareDisplayInformation");
  g_displays.clear();

  // Use a callback to go through each monitor
  if (0 == EnumDisplayMonitors(NULL, NULL, PopulateVirtMonitorBounds, NULL)) {
    throw std::runtime_error("failed to enumerate displays");
  }
  if (g_displays.size() == 0) {
    throw std::runtime_error("0 displays enumerated");
  }

  // in the event that the top-left most point may start out above or below 0, 0
  Pixels origin_offset_x_ = g_displays[0][0]; // left
  Pixels origin_offset_y_ = g_displays[0][2]; // top
  for (const auto& d : g_displays) {
    origin_offset_x_ = d[0] ? d[0] < origin_offset_x_ : origin_offset_x_;
    origin_offset_y_ = d[2] ? d[2] < origin_offset_y_ : origin_offset_y_;
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
    g_displays,
    origin_offset_x_,
    origin_offset_y_,
    USHORT_MAX_VAL / static_cast<double>(virtual_desktop_width),
    USHORT_MAX_VAL / static_cast<double>(virtual_desktop_height),
    GetSystemMetrics(SM_CXVIRTUALSCREEN),
    GetSystemMetrics(SM_CYVIRTUALSCREEN),
  };
}
