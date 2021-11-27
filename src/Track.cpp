/*
 * The release version is designed to run as an administrator
 * UAC Execution Level /level=requireAdministrator
 * The debug version leaves UAC alone.
 *
 * I'm building to x64 primarily because fmt library
 * only 64-bit.
 */

#pragma warning(disable : 4996)
#include "Track.h"

#include "Config.h"
#include "Display.h"
#include "Exceptions.h"
#include "Log.h"
#include "NPClient.h"
#include "Watchdog.h"

// SendInput with absolute mouse movement takes a short int
#define USHORT_MAX_VAL 65535

// bool g_bPauseTracking = false;
bool g_bGracefullyExit = false;

std::vector<CDisplay> g_displays;

signed int g_virtualOriginX = 0;
signed int g_virtualOriginY = 0;
float g_xPixelAbsoluteSlope = 0;
float g_yPixelAbsoluteSlope = 0;

HANDLE g_hWatchdogThread = NULL;

// Uncomment this line for testing to prevent program
// from attaching to NPTrackIR and supersede control
//#define TEST_NO_TRACK

// Function Prototypes
BOOL PopulateVirtMonitorBounds(HMONITOR, HDC, LPRECT);
void WinSetup(CConfig);
void DisplaySetup(const CConfig);
void MouseMove(int, float, float);

void TR_Initialize(HWND hWnd, CConfig config) {
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

  // LogToWix(fmt::format("\nStarting Initialization Of TrackIR\n"));

  WinSetup(config);

  DisplaySetup(config);

  // After settings are loaded, start accepting connections & msgs
  // on a named pipe to externally controll the track IR process
  // Start the watchdog thread
  // if (config.m_bWatchdog)
  if (config.GetBool("General/watchdog_enabled")) {
    // Watchdog thread may return NULL
    g_hWatchdogThread = WatchDog::StartWatchdog();

    if (NULL == g_hWatchdogThread)
      LogToWixError("Watchdog thread failed to initialize!");
  }

  // LogToWix(fmt::format("\n{:-^50}\n", "TrackIR Init Status"));

  // Find and load TrackIR DLL
#ifdef UNICODE
  TCHAR sDll[MAX_PATH];

  int resultConvert =
      MultiByteToWideChar(CP_UTF8,
                          // MB_ERR_INVALID_CHARS, // I feel like this should be
                          // the smart choice, but this is an error.
                          MB_COMPOSITE, config.m_sTrackIrDllLocation.c_str(),
                          MAX_PATH, sDll, MAX_PATH);

  if (0 == resultConvert)
    throw Exception(fmt::format(
        "failed to convert track dll location to wchart* with error code: {}",
        GetLastError()));

#else
  TCHAR sDLL = config.m_sTrackIrDllLocation.c_str()

#endif

  // Load trackir dll and resolve function addresses
  NPRESULT rsltInit = NPClient_Init(sDll);

  if (NP_OK == rsltInit)
    LogToFile("NP Initialization: Success");
  else if (NP_ERR_DLL_NOT_FOUND == rsltInit) {
    LogToFile(
        fmt::format("NP Initialization: Failure, Error Code -> {}", rsltInit));
    throw Exception(
        fmt::format("\nFAILED TO LOAD TRACKIR DLL : {}\n\n", rsltInit));
  } else {
    LogToFile(fmt::format("NP Initialization: Failure, Error Code -> {}\n",
                          rsltInit));
    throw Exception(
        fmt::format("\nNP INITIALIZATION FAILED WITH CODE: {}\n\n", rsltInit));
  }

  // NP software needs a window handle to know when it should
  // stop sending data frames if window is closed.
  HWND hConsole = hWnd;

  // So that this program doesn't boot control of NP software
  // from my local MouseTrackIR instance.
#ifndef TEST_NO_TRACK
  // NP_RegisterWindowHandle
  NPRESULT rsltRegWinHandle = NP_RegisterWindowHandle(hConsole);

  // 7 is a magic number I found through experimentation.
  // It means another program has its window handle registered
  // already.
  if (rsltRegWinHandle == 7) {
    NP_UnregisterWindowHandle();
    LogToWixError(
        fmt::format("BOOTING CONTROL OF PREVIOUS MOUSETRACKER INSTANCE!"));
    Sleep(2);
    rsltRegWinHandle = NP_RegisterWindowHandle(hConsole);
  }

  if (NP_OK == rsltRegWinHandle)
    LogToFile("Register Window Handle: Success");
  else
    throw Exception(
        fmt::format("Register Window Handle: Failed {}\n", rsltRegWinHandle));

  // I'm skipping query the software version, I don't think its necessary

  // Request roll, pitch. See NPClient.h
  NPRESULT rsltReqData = NP_RequestData(NPPitch | NPYaw);
  if (NP_OK == rsltReqData)
    LogToFile("Request Data:  Success");
  else
    throw Exception(fmt::format("Request Data: Failed: {}", rsltReqData));

  rsltReqData =
      NP_RegisterProgramProfileID(config.m_activeProfile.m_profile_ID);
  if (NP_OK == rsltReqData)
    LogToFile("Register Profile ID: Success");
  else
    throw Exception(
        fmt::format("Register Profile ID: Failed: {}\n", rsltReqData));

#endif
  return;
}

int TR_TrackStart(CConfig config) {
  g_bGracefullyExit = false;
  
#ifndef TEST_NO_TRACK
  // Skipping this api call. I think this is for legacy games.
  // NP_StopCursor

  NPRESULT rslt = NP_StartDataTransmission();
  if (NP_OK == rslt)
    LogToFile("Start Data Transmission: Success");
  else
    LogToWixError(fmt::format("Start Data Transmission: Failed: {}\n", rslt));

#endif

  NPRESULT gdf;
  tagTrackIRData *pTIRData, TIRData;
  pTIRData = &TIRData;

  unsigned short lastFrame = 0;
  // used for testing, dropped frames rare and not real world relevant
  // int droppedFrames = 0;

  while (true) {
    gdf = NP_GetData(pTIRData);
    if (NP_OK == gdf) {
      // unsigned short status = (*pTIRData).wNPStatus;
      unsigned short framesig = (*pTIRData).wPFrameSignature;
      // TODO: apply negative sign on startup to avoid extra operation here
      // yaw and pitch come reversed for some reason from trackIR
      float yaw = -(*pTIRData).fNPYaw;
      float pitch = -(*pTIRData).fNPPitch;

      // Don't move the mouse when TrackIR is paused
      if (framesig == lastFrame) {
        // TrackIR 5 supposedly operates at 120hz
        // 8ms is approximately 1 frame
        Sleep(8);
        continue;
      }

      if (WatchDog::g_bPauseTracking == false)
        MouseMove(config.m_monitorCount, yaw, pitch);

      if (g_bGracefullyExit)
      {
        return 0;
      }

      lastFrame = framesig;
    }

    else if (NP_ERR_DEVICE_NOT_PRESENT == gdf) {
      LogToWixError(
          "DEVICE NOT PRESENT\nSTOPPING TRACKING...\nPLEASE RESTART "
          "PROGRAM");
      return 1;
    }

    Sleep(8);
  }

  return 0;
}

void TR_TrackStop() {
  g_bGracefullyExit = true;
  NP_StopDataTransmission();
  NP_UnregisterWindowHandle();
}

BOOL PopulateVirtMonitorBounds(HMONITOR hMonitor, HDC hdcMonitor,
                               LPRECT lprcMonitor, LPARAM lParam) {
  static int count{0};
  MONITORINFOEX Monitor;
  Monitor.cbSize = sizeof(MONITORINFOEX);
  GetMonitorInfo(hMonitor, &Monitor);

  // Monitor Pixel Bounds in the Virtual Desktop
  // static_cast is long -> signed int
  signed int left = static_cast<unsigned int>(Monitor.rcMonitor.left);
  signed int right = static_cast<unsigned int>(Monitor.rcMonitor.right);
  signed int top = static_cast<unsigned int>(Monitor.rcMonitor.top);
  signed int bottom = static_cast<unsigned int>(Monitor.rcMonitor.bottom);

  // CDisplay may create an extra copy constructor, but I want to guarantee
  // overload resolution uses my initializer list to instantiate a single item.
  // TODO: Optimize this later, see Scott Meyers notes about overload resolution
  // and vectors.
  g_displays.push_back(CDisplay{left, right, top, bottom});

  // Display monitor info to user
  // TODO: michael does not yet support wide character strings
  // LogToWix(fmt::format(L"MON Name:{:>15}\n", Monitor.szDevice));

  LogToFile(fmt::format("MON {} Pixel Bounds -> {:>6}, {:>6}, {:>6}, {:>6}",
                        count, left, right, top, bottom));

  if (Monitor.rcMonitor.left < g_virtualOriginX) {
    g_virtualOriginX = Monitor.rcMonitor.left;
  }
  if (Monitor.rcMonitor.top < g_virtualOriginY) {
    g_virtualOriginY = Monitor.rcMonitor.top;
  }

  count++;
  return true;
};

void WinSetup(CConfig config) {
  // TODO: error checking
  //  - rotations over 180/-180 deg
  //  - differing # of displays
  //  - interfering boundaries
  //  -
  // LogToWix(fmt::format("\n{:-^50}\n", "Windows Environment Info"));

  const int monitorCount = GetSystemMetrics(SM_CMONITORS);

  int count = config.GetActiveProfileDisplayCount();
  if (monitorCount != count) {
    throw Exception(fmt::format(
        "Incompatible config: {} monitors specified but {} monitors found",
        count, monitorCount));
  }

  int virtualDesktopWidth = GetSystemMetrics(
      SM_CXVIRTUALSCREEN);  // width of total bounds of all screens
  int virtualDesktopHeight = GetSystemMetrics(
      SM_CYVIRTUALSCREEN);  // height of total bounds of all screens

  // TODO: this check actually happens at the initialization of my configuration
  // file
  LogToFile(fmt::format("{} Monitors Found", monitorCount));
  LogToFile(
      fmt::format("Width of Virtual Desktop:  {:>5}", virtualDesktopWidth));
  LogToFile(
      fmt::format("Height of Virtual Desktop: {:>5}", virtualDesktopHeight));

  g_xPixelAbsoluteSlope =
      USHORT_MAX_VAL / static_cast<float>(virtualDesktopWidth);
  g_yPixelAbsoluteSlope =
      USHORT_MAX_VAL / static_cast<float>(virtualDesktopHeight);

  // LogToFile("\nVirtual Desktop Pixel Bounds\n");
  EnumDisplayMonitors(NULL, NULL, PopulateVirtMonitorBounds, 0);

  LogToFile(
      fmt::format("Virtual Origin Offset Horizontal: {:d}", g_virtualOriginX));
  LogToFile(
      fmt::format("Virtual Origin Offset Vertical:   {:d}", g_virtualOriginY));

  return;
}

void DisplaySetup(const CConfig config) {
  for (int i = 0; i < config.m_activeProfile.m_bounds.size(); i++) {
    g_displays[i].rotationBoundLeft =
        config.m_activeProfile.m_bounds[i].rotationBounds[0];
    g_displays[i].rotationBoundRight =
        config.m_activeProfile.m_bounds[i].rotationBounds[1];
    g_displays[i].rotationBoundTop =
        config.m_activeProfile.m_bounds[i].rotationBounds[2];
    g_displays[i].rotationBoundBottom =
        config.m_activeProfile.m_bounds[i].rotationBounds[3];

    g_displays[i].paddingLeft =
        config.m_activeProfile.m_bounds[i].paddingBounds[0];
    g_displays[i].paddingRight =
        config.m_activeProfile.m_bounds[i].paddingBounds[1];
    g_displays[i].paddingTop =
        config.m_activeProfile.m_bounds[i].paddingBounds[2];
    g_displays[i].paddingBottom =
        config.m_activeProfile.m_bounds[i].paddingBounds[3];

    g_displays[i].setAbsBounds(g_virtualOriginX, g_virtualOriginY,
                               g_xPixelAbsoluteSlope, g_yPixelAbsoluteSlope);
  }

  for (int i = 0; i < config.m_monitorCount; i++) {
    LogToFile(fmt::format(
        "MON {} pixelBound: {:>5d}, {:>5d}, {:>5d}, {:>5d}", i,
        g_displays[i].pixelBoundAbsLeft, g_displays[i].pixelBoundAbsRight,
        g_displays[i].pixelBoundAbsTop, g_displays[i].pixelBoundAbsBottom));
  }

  for (int i = 0; i < config.m_monitorCount; i++) {
    LogToFile(fmt::format(
        "MON {} boundAbsLeft: {:>7.1f}, {:>7.1f}, {:>7.1f}, {:>7.1f}", i,
        g_displays[i].boundAbsLeft, g_displays[i].boundAbsRight,
        g_displays[i].boundAbsTop, g_displays[i].boundAbsBottom));
  }

  for (int i = 0; i < config.m_monitorCount; i++) {
    LogToFile(fmt::format(
        "MON {} rotationBoundLeft: {:> 6.2f}, {:> 6.2f}, {:> 6.2f}, {:> 6.2f}",
        i, g_displays[i].rotationBoundLeft, g_displays[i].rotationBoundRight,
        g_displays[i].rotationBoundTop, g_displays[i].rotationBoundBottom));
  }

  return;
}

inline void SendMyInput(float x, float y) {
  static MOUSEINPUT mi = {
      0, 0,
      0, MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK,
      0, 0};

  static INPUT ip = {INPUT_MOUSE, mi};

  ip.mi.dx = static_cast<LONG>(x);
  ip.mi.dy = static_cast<LONG>(y);

  SendInput(1, &ip, sizeof(INPUT));

  return;
}

void MouseMove(int monitorCount, float yaw, float pitch) {
  // set the last screen initially equal to the main display for me
  // TODO: find a way to specify in settings or get from windows
  static int lastScreen = 0;

  // variables used for linear interpolation
  static float rl;
  static float al;
  static float mx;
  static float x;
  static float rt;
  static float at;
  static float my;
  static float y;

  // Check if the head is pointing to a screen
  // The return statement is never reached if the head is pointing outside the
  // bounds of any of the screens

  for (int i = 0; i <= monitorCount - 1; i++) {
    if ((yaw > g_displays[i].rotationBound16BitLeft) &&
        (yaw < g_displays[i].rotationBound16BitRight) &&
        (pitch < g_displays[i].rotationBound16BitTop) &&
        (pitch > g_displays[i].rotationBound16BitBottom)) {
      // I wrote it out for maintainability
      // its plenty fast anyway for a 60hz limited display
      rl = g_displays[i].rotationBound16BitLeft;
      al = g_displays[i].boundAbsLeft;
      mx = g_displays[i].xSlope;
      x = mx * (yaw - rl) + al;
      rt = g_displays[i].rotationBound16BitTop;
      at = g_displays[i].boundAbsTop;
      my = g_displays[i].ySlope;
      y = my * (rt - pitch) + at;
      // load the coordinates into my input structure
      // need to cast to an integer because resulting calcs are floats
      SendMyInput(x, y);
      lastScreen = i;
      // LogToWix(fmt::format("(%f, %f)", y, x); // for testing
      return;
    }
  }

  // If the head is pointing outside of the bounds of a screen the mouse
  // should snap to the breached edge it could either be the pitch or the
  // yaw axis that is too great or too little to do this assume the pointer
  // came from the last screen, just asign the mouse position to the absolute
  // limit from the screen it came from.
  if (yaw < g_displays[lastScreen].rotationBound16BitLeft) {
    x = g_displays[lastScreen].boundAbsLeft +
        g_displays[lastScreen].paddingLeft * g_xPixelAbsoluteSlope;
  } else if (yaw > g_displays[lastScreen].rotationBound16BitRight) {
    x = g_displays[lastScreen].boundAbsRight -
        g_displays[lastScreen].paddingRight * g_xPixelAbsoluteSlope;
  } else {
    rl = g_displays[lastScreen].rotationBound16BitLeft;
    al = g_displays[lastScreen].boundAbsLeft;
    mx = g_displays[lastScreen].xSlope;
    x = mx * (yaw - rl) + al;
  }

  if (pitch > g_displays[lastScreen].rotationBound16BitTop) {
    y = g_displays[lastScreen].boundAbsTop +
        g_displays[lastScreen].paddingTop * g_yPixelAbsoluteSlope;
  } else if (pitch < g_displays[lastScreen].rotationBound16BitBottom) {
    y = g_displays[lastScreen].boundAbsBottom -
        g_displays[lastScreen].paddingBottom * g_yPixelAbsoluteSlope;
  } else {
    rt = g_displays[lastScreen].rotationBound16BitTop;
    at = g_displays[lastScreen].boundAbsTop;
    my = g_displays[lastScreen].ySlope;
    y = my * (rt - pitch) + at;
  }

  SendMyInput(x, y);

  return;
}
