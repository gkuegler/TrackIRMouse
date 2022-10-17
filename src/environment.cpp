#include "environment.hpp"

#include <Windows.h>
#include <spdlog/spdlog.h>

namespace env {

BOOL
PopulateVirtMonitorBounds(HMONITOR hMonitor,
                          HDC hdcMonitor,
                          LPRECT lprcMonitor,
                          LPARAM lParam)
{
  auto displays = reinterpret_cast<HardwareDisplays*>(lParam);
  MONITORINFOEX monitor;
  monitor.cbSize = sizeof(MONITORINFOEX);
  GetMonitorInfo(hMonitor, &monitor);

  // Monitor Pixel Bounds in the Virtual Desktop
  // static_cast: long -> signed int
  auto left = static_cast<signed int>(monitor.rcMonitor.left);
  auto right = static_cast<signed int>(monitor.rcMonitor.right);
  auto top = static_cast<signed int>(monitor.rcMonitor.top);
  auto bottom = static_cast<signed int>(monitor.rcMonitor.bottom);

  displays->push_back({ left, right, top, bottom });

  // LogToFile(fmt::format("MON {} Pixel Bounds -> {:>6}, {:>6}, {:>6}, {:>6}",
  //                       count, left, right, top, bottom));
  return true;
};

HardwareDisplayInfo
GetHardwareDisplayInfo()
{
  HardwareDisplayInfo hdi;

  const int monitor_count = GetSystemMetrics(SM_CMONITORS);

  // get hieght and width_ of virtual desktop
  hdi.width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  hdi.height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

  spdlog::debug("{} Monitors Found", monitor_count);
  spdlog::debug("Width of Virtual Desktop:  {:>5}", hdi.width);
  spdlog::debug("Height of Virtual Desktop: {:>5}", hdi.height);

  // Use a callback to go through each monitor
  EnumDisplayMonitors(NULL,
                      NULL,
                      PopulateVirtMonitorBounds,
                      reinterpret_cast<LPARAM>(&hdi.displays));
  return hdi;
}
} // namespace env
