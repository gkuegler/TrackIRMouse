/**
 * Interface to start tracking with TrackIR.
 *
 * The release version is designed to run as an administrator
 * UAC Execution Level /level=requireAdministrator
 * The debug version leaves UAC alone and runs as a user.
 *
 * I'm building to x64 primarily because fmt library
 * only 64-bit.
 */

#pragma warning(disable : 4996)
#include "track.hpp"

#include "config.hpp"
#include "display.hpp"
#include "exceptions.hpp"
#include "log.hpp"
#include "np-client.h"
#include "watchdog.hpp"

// Uncomment this line for testing to prevent program
// from attaching to NPTrackIR and supersede control
//#define TEST_NO_TRACK

// SendInput with absolute mouse movement flag takes a short int
constexpr auto USHORT_MAX_VAL = 65535;

// Tracking loop uses this to check if it should break, return to thread, then
// have thread auto clean up
std::atomic<bool> g_bTrackingAllowedToRun = false;

std::vector<CDisplay> g_displays;

signed int g_virtualOriginX = 0;
signed int g_virtualOriginY = 0;
double g_xPixelAbsoluteSlope = 0;
double g_yPixelAbsoluteSlope = 0;

HANDLE g_hWatchdogThread = NULL;

BOOL PopulateVirtMonitorBounds(HMONITOR hMonitor, HDC hdcMonitor,
                               LPRECT lprcMonitor, LPARAM lParam) {
  static int count{0};
  MONITORINFOEX Monitor;
  Monitor.cbSize = sizeof(MONITORINFOEX);
  if (!GetMonitorInfo(hMonitor, &Monitor)) {
    spdlog::warn("Couldn't get display info for display #: {}", count);
    return true;
  }

  // Monitor Pixel Bounds in the Virtual Desktop
  // static_cast: long -> signed int
  auto left = static_cast<signed int>(Monitor.rcMonitor.left);
  auto right = static_cast<signed int>(Monitor.rcMonitor.right);
  auto top = static_cast<signed int>(Monitor.rcMonitor.top);
  auto bottom = static_cast<signed int>(Monitor.rcMonitor.bottom);

  // CDisplay may create an extra copy constructor, but I want to guarantee
  // overload resolution uses my initializer list to instantiate a single item.
  // TODO: Optimize this later, see Scott Meyers notes about overload resolution
  // and vectors.
  g_displays.push_back(CDisplay{left, right, top, bottom});

  // Display monitor info to user
  // TODO: michael does not yet support wide character strings
  // spdlog::info(L"MON Name:{:>15}\n", Monitor.szDevice);

  spdlog::info("MON {} Pixel Bounds -> {:>6}, {:>6}, {:>6}, {:>6}", count, left,
               right, top, bottom);

  if (Monitor.rcMonitor.left < g_virtualOriginX) {
    g_virtualOriginX = Monitor.rcMonitor.left;
  }
  if (Monitor.rcMonitor.top < g_virtualOriginY) {
    g_virtualOriginY = Monitor.rcMonitor.top;
  }

  count++;
  return true;
}
int WinSetup(CConfig config) {
  spdlog::trace("entering WinSetup");

  const int monitorCount = GetSystemMetrics(SM_CMONITORS);

  int count = config.GetActiveProfileDisplayCount();

  if (monitorCount != count) {
    spdlog::error(
        "Incompatible config: {} monitors specified but {} monitors found",
        count, monitorCount);
    return -1;
  }

  int virtualDesktopWidth = GetSystemMetrics(
      SM_CXVIRTUALSCREEN);  // width of total bounds of all screens
  int virtualDesktopHeight = GetSystemMetrics(
      SM_CYVIRTUALSCREEN);  // height of total bounds of all screens

  spdlog::debug("{} Monitors Found", monitorCount);
  spdlog::debug("Width of Virtual Desktop:  {:>5}", virtualDesktopWidth);
  spdlog::debug("Height of Virtual Desktop: {:>5}", virtualDesktopHeight);

  g_xPixelAbsoluteSlope =
      USHORT_MAX_VAL / static_cast<double>(virtualDesktopWidth);
  g_yPixelAbsoluteSlope =
      USHORT_MAX_VAL / static_cast<double>(virtualDesktopHeight);

  // Use a callback to go through each monitor
  EnumDisplayMonitors(NULL, NULL, PopulateVirtMonitorBounds, 0);

  spdlog::debug("Virtual Origin Offset Horizontal: {:>5d}", g_virtualOriginX);
  spdlog::debug("Virtual Origin Offset Vertical:   {:>5d}", g_virtualOriginY);

  return SUCCESS;
}

int DisplaySetup(CConfig config) {
  SProfile activeProfile = config.GetActiveProfile();

  if (false == ValidateUserInput(activeProfile.bounds)) {
    return FAILURE;
  }

  spdlog::trace("user input validated");

  for (int i = 0; i < activeProfile.bounds.size(); i++) {
    // transfer config data to internal strucuture
    for (int j = 0; j < 4; j++) {
      g_displays[i].rotation[j] = activeProfile.bounds[i].rotationBounds[j];
      g_displays[i].padding[j] = activeProfile.bounds[i].paddingBounds[j];
    }

    g_displays[i].setAbsBounds(g_virtualOriginX, g_virtualOriginY,
                               g_xPixelAbsoluteSlope, g_yPixelAbsoluteSlope);
  }

  // debug messages only
  for (int i = 0; i < config.m_monitorCount; i++) {
    spdlog::info(
        "Display {} user rotations: {:>.2f}, {:>.2f}, {:>.2f}, {:>.2f}", i,
        g_displays[i].rotation[0], g_displays[i].rotation[1],
        g_displays[i].rotation[2], g_displays[i].rotation[3]);
  }
  for (int i = 0; i < config.m_monitorCount; i++) {
    spdlog::debug("Display {} pixel bounds: {}, {}, {}, {}", i,
                  g_displays[i].absPixel[0], g_displays[i].absPixel[1],
                  g_displays[i].absPixel[2], g_displays[i].absPixel[3]);
  }
  for (int i = 0; i < config.m_monitorCount; i++) {
    spdlog::info(
        "Display {} absolute bounds: {:>.1f}, {:>.1f}, {:>.1f}, {:>.1f}", i,
        g_displays[i].absCached[0], g_displays[i].absCached[1],
        g_displays[i].absCached[2], g_displays[i].absCached[3]);
  }

  return SUCCESS;
}

int TR_Initialize(HWND hWnd, CConfig config) {
  // ## Program flow ##
  //
  // first thing is ping windows for info
  // validate settings and build display structures
  // start watchdog thread
  // load trackir dll and getproc addresses
  // connect to trackir and request data type
  // receive data frames
  // decode data
  // hand data off to track program
  // calculate mouse position
  // form & send mouse move command

  // Error Conditions To Check For RAII:
  //  - unable to initialize thread
  //  - unable to load dll
  //  - any NP dll function call failure

  spdlog::trace("Starting Initialization Of TrackIR");

  SProfile activeProfile = config.GetActiveProfile();

  if (FAILURE == WinSetup(config)) {
    return FAILURE;
  }
  spdlog::trace("win setup a success");

  if (FAILURE == DisplaySetup(config)) {
    return FAILURE;
  }

  spdlog::trace("display setup a success");

  // After settings are loaded, start accepting connections & msgs
  // on a named pipe to externally controll the track IR process
  // Start the watchdog thread
  // if (config.m_bWatchdog)
  if (config.data.watchdogEnabled) {
    // Watchdog thread may return NULL
    g_hWatchdogThread = WatchDog::StartWatchdog();

    if (NULL == g_hWatchdogThread)
      spdlog::warn("Watchdog thread failed to initialize.");
  }

  // Find and load TrackIR DLL
#ifdef UNICODE
  TCHAR sDll[MAX_PATH];

  int resultConvert = MultiByteToWideChar(
      CP_UTF8,
      // MB_ERR_INVALID_CHARS, // I feel like this should be
      // the smart choice, but this is an error.
      MB_COMPOSITE, config.m_trackIrDllPath.c_str(), MAX_PATH, sDll, MAX_PATH);

  if (0 == resultConvert) {
    spdlog::error(
        "failed to convert track dll location to wchart* with error code: {}",
        GetLastError());
    return FAILURE;
  }

#else
  TCHAR sDLL = config.m_trackIrDllPath.c_str()
#endif

  // Load trackir dll and resolve function addresses
  NPRESULT rsltInit = NPClient_Init(sDll);

  if (NP_OK == rsltInit) {
    spdlog::info("NP Initialization: Success");
  } else {
    // logging handled within the function implementation
    return FAILURE;
  }

  // NP software needs a window handle to know when it should
  // stop sending data frames if window is closed.
  HWND hConsole = hWnd;

  // So that this program doesn't boot control of NP software
  // from my local MouseTrackIR instance.
#ifndef TEST_NO_TRACK
  // NP_RegisterWindowHandle
  NPRESULT rsltRegWinHandle = NP_RegisterWindowHandle(hConsole);

  if (NP_OK == rsltRegWinHandle) {
    spdlog::info("Registered window handle.");
  }

  // 7 is a magic number I found through experimentation.
  // It means another program has its window handle registered
  // already.
  else if (rsltRegWinHandle == 7) {
    NP_UnregisterWindowHandle();
    spdlog::warn("Booting control of previous mousetracker instance.");
    Sleep(2);
    rsltRegWinHandle = NP_RegisterWindowHandle(hConsole);
    if (!NP_OK == rsltRegWinHandle) {
      spdlog::error("Failed to re-register window handle with NP code: {}",
                    rsltRegWinHandle);
      return FAILURE;
    }
  } else {
    spdlog::error("Failed to register window handle with NP code: {}",
                  rsltRegWinHandle);
    return FAILURE;
  }

  // I'm skipping query the software version, I don't think its necessary

  // Request roll, pitch. See NPClient.h
  NPRESULT rsltReqData = NP_RequestData(NPPitch | NPYaw);
  if (NP_OK == rsltReqData)
    spdlog::info("NP Request Data Success");
  else {
    spdlog::error("NP Request Data failed with NP code: {}", rsltReqData);
    return FAILURE;
  }

  NPRESULT rsltProfileId =
      NP_RegisterProgramProfileID(activeProfile.profile_ID);
  if (NP_OK == rsltProfileId)
    spdlog::info("NP Registered Profile ID.");
  else {
    spdlog::error("NP Register Profile ID failed with NP code: {}",
                  rsltProfileId);
    return FAILURE;
  }

#endif
  return 0;
}

inline void SendMyInput(double x, double y) {
  static MOUSEINPUT mi = {
      0, 0,
      0, MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK,
      0, 0};

  static INPUT ip = {INPUT_MOUSE, mi};

  ip.mi.dx = static_cast<LONG>(x);
  ip.mi.dy = static_cast<LONG>(y);

  if (0 == SendInput(1, &ip, sizeof(INPUT))) {
    spdlog::warn("SendInput was already blocked by another thread.");
  }

  return;
}
void MouseMove(double yaw, double pitch) {
  static int lastScreen = 0;

  // Check if the head is pointing to a screen
  // The return statement is never reached if the head is pointing outside the
  // bounds of any of the screens
  for (int i = 0; i < g_displays.size(); i++) {
    double rl = g_displays[i].rotation16bit[0];
    double rr = g_displays[i].rotation16bit[1];
    double rt = g_displays[i].rotation16bit[2];
    double rb = g_displays[i].rotation16bit[3];
    if ((yaw > rl) && (yaw < rr) && (pitch < rt) && (pitch > rb)) {
      // interpolate horizontal position from left edge of display
      double al = g_displays[i].absCached[0];
      double mx = g_displays[i].xSlope;
      double x = mx * (yaw - rl) + al;

      // interpolate vertical position from top edge of display
      double at = g_displays[i].absCached[2];
      double my = g_displays[i].ySlope;
      double y = my * (rt - pitch) + at;

      SendMyInput(x, y);
      lastScreen = i;
      return;
    }
  }

  // If the head is pointing outside of the bounds of a screen the mouse
  // should snap to the breached edge it could either be the pitch or the
  // yaw axis that is too great or too little to do this assume the pointer
  // came from the last screen, just asign the mouse position to the absolute
  // limit from the screen it came from.
  static double x;
  static double y;

  double rl = g_displays[lastScreen].rotation16bit[0];
  double rr = g_displays[lastScreen].rotation16bit[1];
  double rt = g_displays[lastScreen].rotation16bit[2];
  double rb = g_displays[lastScreen].rotation16bit[3];

  if (yaw < rl) {  // horizontal rotaion is left of last used display
    x = g_displays[lastScreen].absCached[0] +
        g_displays[lastScreen].padding[0] * g_xPixelAbsoluteSlope;
  } else if (yaw > rr) {  // horizontal rotation is right of last used display
    x = g_displays[lastScreen].absCached[1] -
        g_displays[lastScreen].padding[1] * g_xPixelAbsoluteSlope;
  } else {  // horizontal roation within last display bounds as normal
    double al = g_displays[lastScreen].absCached[0];
    double mx = g_displays[lastScreen].xSlope;
    x = mx * (yaw - rl) + al;  // interpolate horizontal position from left
                               // edge of display
  }

  if (pitch > rt) {  // vertical rotaion is above last used display
    y = g_displays[lastScreen].absCached[2] +
        g_displays[lastScreen].padding[2] * g_yPixelAbsoluteSlope;
  } else if (pitch < rb) {  // vertical rotaion is last used display
    y = g_displays[lastScreen].absCached[3] -
        g_displays[lastScreen].padding[3] * g_yPixelAbsoluteSlope;
  } else {
    double at = g_displays[lastScreen].absCached[2];
    double my = g_displays[lastScreen].ySlope;
    y = my * (rt - pitch) + at;  // interpolate vertical position from top edge
                                 // of display
  }

  SendMyInput(x, y);
  return;
}
int TR_TrackStart(CConfig config) {
  g_bTrackingAllowedToRun = true;

#ifndef TEST_NO_TRACK
  // Skipping this api call. I think this is for legacy games.
  // NP_StopCursor

  NPRESULT rslt = NP_StartDataTransmission();
  if (NP_OK == rslt)
    spdlog::info("NP Started data transmission.");
  else {
    spdlog::error("NP Start Data Transmission failed with code: {}\n", rslt);
    return 1;
  }

#endif

  NPRESULT gdf;
  tagTrackIRData *pTIRData, TIRData;
  pTIRData = &TIRData;

  unsigned short lastFrame = 0;
  // used for testing, dropped frames rare and not real world relevant
  // int droppedFrames = 0;

  while (g_bTrackingAllowedToRun) {
    gdf = NP_GetData(pTIRData);
    if (NP_OK == gdf) {
      // unsigned short status = (*pTIRData).wNPStatus;
      unsigned short framesig = (*pTIRData).wPFrameSignature;
      // TODO: apply negative sign on startup to avoid extra operation here
      // yaw and pitch come reversed relative to GUI profgram for some reason
      // from trackIR
      double yaw = (-(*pTIRData).fNPYaw);      // implicit float to double
      double pitch = (-(*pTIRData).fNPPitch);  // implicit float to double

      // Don't move the mouse when TrackIR is paused
      if (framesig == lastFrame) {
        // TrackIR 5 supposedly operates at 120hz
        // 8ms is approximately 1 frame
        Sleep(8);
        continue;
      }

      // Watchdog enables software to be controlled via a named pipe. This is
      // primarily used during testing so that my test instance can latch on to
      // active tracking data without re-registering a window handle. A disbale
      // msg is sent before my test instance launches, then my normal instance
      // is enables after as part of my build script.
      if (WatchDog::g_bPauseTracking == false) {
        MouseMove(yaw, pitch);
      }

      lastFrame = framesig;
    }

    else if (NP_ERR_DEVICE_NOT_PRESENT == gdf) {
      spdlog::warn("device not present, tracking stopped");
      return 1;
    }

    Sleep(8);
  }

  return 0;
}

void TR_TrackStop() {
  g_bTrackingAllowedToRun = false;
  NP_StopDataTransmission();
  NP_UnregisterWindowHandle();
}
