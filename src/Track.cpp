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

#include "NPClient.h"
#include "Display.h"
#include "Watchdog.h"
#include "Exceptions.h"
#include "Config.h"
#include "Log.h"

#include <wx/wx.h>

#define USHORT_MAX_VAL 65535 // SendInput with absolute mouse movement takes a short int

bool g_bPauseTracking = false;

// Uncomment this line for testing to prevent program
// from attaching to NPTrackIR and supersede control
//#define TEST_NO_TRACK

//void disconnectTrackIR(void);


CTracker::CTracker(wxEvtHandler* m_parent, HWND hWnd, CConfig config)
{

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

	//Error Conditions To Check For RAII:
	// - unable to initialize thread
	// - unable to load dll
	// - any NP dll function call failure



	LogToWix(fmt::format("\nStarting Initialization Of TrackIR\n"));

	WinSetup(config);
	
	DisplaySetup(config);

	// After settings are loaded, start accepting connections & msgs
	// on a named pipe to externally controll the track IR process
	// Start the watchdog thread
	// if (config.m_bWatchdog)
	if (config.GetBool("General/watchdog_enabled"))
	{
		// Watchdog thread may return NULL
		m_hWatchdogThread = WatchDog::StartWatchdog();

		if (NULL == m_hWatchdogThread)
		{
			LogToWix("Watchdog thread failed to initialize!");
		}
	}

	LogToWix(fmt::format("\n{:-^50}\n", "TrackIR Init Status"));

	// Find and load TrackIR DLL
	TCHAR sDll[MAX_PATH];
	std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> convert;
	std::wstring wide_string = convert.from_bytes(config.m_sTrackIrDllLocation);
	wcscpy_s(sDll, MAX_PATH, wide_string.c_str());

	// Load trackir dll and resolve function addresses
	NPRESULT rslt = NP_OK;
	rslt = NPClient_Init(sDll);

	if (NP_OK == rslt)
	{
		LogToWix("NP Initialization:          Success\n");
	}
	else if (NP_ERR_DLL_NOT_FOUND == rslt)
	{
		LogToWixError(fmt::format("\nFAILED TO LOAD TRACKIR DLL: {}\n\n", rslt));
		throw Exception("Failed to load track IR DLL.");
	}
	else
	{
		LogToWixError(fmt::format("\nNP INITIALIZATION FAILED WITH CODE: {}\n\n", rslt));
		throw Exception("Failed to initialize track IR.");
	}

	// NP needs a window handle to send data frames to
	HWND hConsole = hWnd;

	// In the program early after the DLL is loaded for testing
	// so that this program doesn't boot control
	// from my local copy.
#ifndef TEST_NO_TRACK
	// NP_RegisterWindowHandle
	rslt = NP_OK;
	rslt = NP_RegisterWindowHandle(hConsole);

	// 7 is a magic number I found through experimentation.
	// It means another program has its window handle registered
	// already.
	if (rslt == 7)
	{
		NP_UnregisterWindowHandle();
		LogToWixError(fmt::format("\nBOOTING CONTROL OF PREVIOUS MOUSETRACKER INSTANCE!\n\n"));
		Sleep(2);
		rslt = NP_RegisterWindowHandle(hConsole);
	}

	if (NP_OK == rslt)
	{
		LogToWix("Register Window Handle:      Success\n");
	}
	else
	{
		LogToWixError(fmt::format("Register Window Handle: Failed {:>3}\n", rslt));
		throw Exception("");
	}

	// I'm skipping query the software version, I don't think its necessary

	// Request roll, pitch. See NPClient.h
	rslt = NP_RequestData(NPPitch | NPYaw);
	if (NP_OK == rslt)
	{
		LogToWix("Request Data:         Success\n");
	}
	else
	{
		LogToWixError(fmt::format("Request Data:        Failed: {:>3}\n", rslt));
		throw Exception("");
	}

	rslt = NP_RegisterProgramProfileID(config.m_profile_ID);
	if (NP_OK == rslt)
	{
		LogToWix("Register Profile ID:         Success\n");
	}
	else
	{
		LogToWixError(fmt::format("Register Profile ID:       Failed: {:>3}\n", rslt));
		throw Exception("");
	}

#endif
}

int CTracker::trackStart(CConfig config)
{
#ifndef TEST_NO_TRACK
	// Skipping this too. I think this is for legacy games
	// NP_StopCursor

	NPRESULT rslt = NP_StartDataTransmission();
	if (NP_OK == rslt)
	{
		LogToWix("Start Data Transmission:			Success\n");
	}
	else
	{
		LogToWixError(fmt::format("Start Data Transmission:     Failed: {:>3}\n", rslt));
	}

#endif

	NPRESULT gdf;
	tagTrackIRData* pTIRData, TIRData;
	pTIRData = &TIRData;

	unsigned short lastFrame = 0;
	// used for testing, dropped frames rare and not real world relevant
	// int droppedFrames = 0;

	while(true)
	{
		gdf = NP_GetData(pTIRData);
		if (NP_OK == gdf)
		{
			unsigned short status = (*pTIRData).wNPStatus;
			unsigned short framesig = (*pTIRData).wPFrameSignature;
			// TODO: Make Future optimization to not need a negative sign
			float yaw = -(*pTIRData).fNPYaw;
			float pitch = -(*pTIRData).fNPPitch;

			// Don't try to move the mouse when TrackIR is paused
			if (framesig == lastFrame)
			{
				// LogToWix(fmt::format("Tracking Paused\n");
				// TrackIR5 supposedly operates at 120hz
				// 8ms is approximately 1 frame
				Sleep(8);
				continue;
			}

			if (g_bPauseTracking == false)
			{
				MouseMove(config.m_monitorCount, yaw, pitch);
			}

			// If I would like to log rotational data
			//LogToWix(fmt::format("fNPYaw: {:f}\n", yaw));
			//LogToWix(fmt::format("fNPPitch: {:f}\n", pitch));

			// I don't actually really care about dropped frames
			//if ((framesig != lastFrame + 1) && (lastFrame != 0))
			//{
				//LogToWix("dropped frame");
				//droppedFrames = droppedFrames + framesig - lastFrame - 1;
			//}

			lastFrame = framesig;
		}
		else if (NP_ERR_DEVICE_NOT_PRESENT == gdf)
		{
			LogToWixError("\n\nDEVICE NOT PRESENT\nSTOPPING TRACKING...\nPLEASE RESTART PROGRAM\n\n");
			break;
		}

		Sleep(8);
	}
	
	return 0;
}

void CTracker::trackStop()
{
	NP_StopDataTransmission();
	NP_UnregisterWindowHandle();
}

BOOL CTracker::WrapperPopulateVirtMonitorBounds(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM lParam)
{
	CTracker* pThis = reinterpret_cast<CTracker*>(lParam);
	BOOL winreturn = pThis->PopulateVirtMonitorBounds(hMonitor, hdcMonitor, lprcMonitor);
	return true;
}

BOOL CTracker::PopulateVirtMonitorBounds(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor)
{
	static int count{ 0 };
	MONITORINFOEX Monitor;
	Monitor.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMonitor, &Monitor);

	// Monitor Pixel Bounds in the Virtual Desktop
	// static_cast is long -> signed int
	signed int left   = static_cast<unsigned int>(Monitor.rcMonitor.left);
	signed int right  = static_cast<unsigned int>(Monitor.rcMonitor.right);
	signed int top    = static_cast<unsigned int>(Monitor.rcMonitor.top);
	signed int bottom = static_cast<unsigned int>(Monitor.rcMonitor.bottom);

	// CDisplay may create an extra copy constructor, but I want to guarantee
	// overload resolution uses my initializer list to instantiate a single item.
	// TODO: Optimize this later, see Scott Meyers notes about overload resolutionand vectors.
	m_displays.push_back(CDisplay{ left, right, top, bottom });


	// Display monitor info to user
	//LogToWix(fmt::format(L"MON Name:{:>15}\n", Monitor.szDevice));

	LogToWix(fmt::format("MON {} Left:   {:>10}\n", count, left));
	LogToWix(fmt::format("MON {} Right:  {:>10}\n", count, right));
	LogToWix(fmt::format("MON {} Top:    {:>10}\n", count, top));
	LogToWix(fmt::format("MON {} Bottom: {:>10}\n", count, bottom));

	if (Monitor.rcMonitor.left < m_virtualOriginX)
	{
		m_virtualOriginX = Monitor.rcMonitor.left;
	}
	if (Monitor.rcMonitor.top < m_virtualOriginY)
	{
		m_virtualOriginY = Monitor.rcMonitor.top;
	}

	count++;
	return true;
};

void CTracker::WinSetup(CConfig config)
{
	LogToWix(fmt::format("\n{:-^50}\n", "Windows Environment Info"));

	const int monitorCount = GetSystemMetrics(SM_CMONITORS);
	int virtualDesktopWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);  // width of total bounds of all screens
	int virtualDesktopHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN); // height of total bounds of all screens

	// TODO: this check actually happens at the initialization of my configuration file
	LogToWixError(fmt::format("Displays Specified: {}\nDisplays Found: {}\n", config.m_bounds.size(), monitorCount));

	if (config.m_bounds.size() < monitorCount)
	{
		LogToWixError(fmt::format("More Displays Connected Then Specified In Settings File.\n{} Displays Found.\n{} Displays Specified.\n", monitorCount, config.m_bounds.size()));
	}
	if (config.m_bounds.size() > monitorCount)
	{
		LogToWixError(fmt::format("Less Displays Connected Then Specified In Settings File.\nThe Wrong Configuration May Be Selected.\n{} Displays Found.\n{} Displays Specified.\n", monitorCount, config.m_bounds.size()));
	}

	LogToWix(fmt::format("{} Monitors Found\n", monitorCount));
	LogToWix(fmt::format("Width of Virtual Desktop:  {:>5}\n", virtualDesktopWidth));
	LogToWix(fmt::format("Height of Virtual Desktop: {:>5}\n", virtualDesktopHeight));

	/* #################################################################
	For a future feature to automatically detect monitor configurations
	and select the correct profile.

	DISPLAY_DEVICEA display_device_info;
	display_device_info.cb = sizeof(DISPLAY_DEVICEA);

	for (int i = 0; ; i++)
	{
		BOOL result = EnumDisplayDevicesA(
			0,
			i,
			&display_device_info,
			0
		);

		if (result)
		{
			//std::cout << i << " Name: " << display_device_info.DeviceName << "  DeviceString: " << display_device_info.DeviceString << std::endl;
			LogToWix(fmt::format("{} Name: {} DeviceString: {}\n", i, display_device_info.DeviceName, display_device_info.DeviceString));
		}
		else
		{
			break;
		}
	}
	######################################################################## */

	m_xPixelAbsoluteSlope = USHORT_MAX_VAL / static_cast<float>(virtualDesktopWidth);
	m_yPixelAbsoluteSlope = USHORT_MAX_VAL / static_cast<float>(virtualDesktopHeight);

	LogToWix("\nVirtual Desktop Pixel Bounds\n");
	EnumDisplayMonitors(NULL, NULL, WrapperPopulateVirtMonitorBounds, reinterpret_cast<LPARAM>(this));

	LogToWix(fmt::format("\nVirtual Origin Offset Horizontal: {:d}\n", m_virtualOriginX));
	LogToWix(fmt::format("Virtual Origin Offset Vertical:   {:d}\n", m_virtualOriginY));

	return;
}

void CTracker::DisplaySetup(const CConfig config)
{
	for (int i = 0; i < config.m_monitorCount; i++)
	{
		m_displays[i].rotationBoundLeft   = config.m_bounds[i].rotationBounds[0];
		m_displays[i].rotationBoundRight  = config.m_bounds[i].rotationBounds[1];
		m_displays[i].rotationBoundTop    = config.m_bounds[i].rotationBounds[2];
		m_displays[i].rotationBoundBottom = config.m_bounds[i].rotationBounds[3];

		m_displays[i].paddingLeft         = config.m_bounds[i].paddingBounds[0];
		m_displays[i].paddingRight        = config.m_bounds[i].paddingBounds[1];
		m_displays[i].paddingTop          = config.m_bounds[i].paddingBounds[2];
		m_displays[i].paddingBottom       = config.m_bounds[i].paddingBounds[3];

		m_displays[i].setAbsBounds(m_virtualOriginX, m_virtualOriginY, m_xPixelAbsoluteSlope, m_yPixelAbsoluteSlope);
	}
	LogToWix("\nVirtual Desktop Pixel Bounds (abs)\n");
	for (int i = 0; i < config.m_monitorCount; i++)
	{
		LogToWix(fmt::format("MON {} pixelBoundboundAbsLeft:   {:>10d}\n", i, m_displays[i].pixelBoundAbsLeft));
		LogToWix(fmt::format("MON {} pixelBoundboundAbsRight:  {:>10d}\n", i, m_displays[i].pixelBoundAbsRight));
		LogToWix(fmt::format("MON {} pixelBoundboundAbsTop:    {:>10d}\n", i, m_displays[i].pixelBoundAbsTop));
		LogToWix(fmt::format("MON {} pixelBoundboundAbsBottom: {:>10d}\n", i, m_displays[i].pixelBoundAbsBottom));
	}
	LogToWix("\n16-bit Coordinate Bounds\n");
	for (int i = 0; i < config.m_monitorCount; i++)
	{
		LogToWix(fmt::format("MON {} boundAbsLeft:       {:>12.1f}\n", i, m_displays[i].boundAbsLeft));
		LogToWix(fmt::format("MON {} boundAbsRight:      {:>12.1f}\n", i, m_displays[i].boundAbsRight));
		LogToWix(fmt::format("MON {} boundAbsTop:        {:>12.1f}\n", i, m_displays[i].boundAbsTop));
		LogToWix(fmt::format("MON {} boundAbsBottom:     {:>12.1f}\n", i, m_displays[i].boundAbsBottom));
	}
	LogToWix("\nRotational Bounds\n");
	for (int i = 0; i < config.m_monitorCount; i++)
	{
		LogToWix(fmt::format("MON {} rotationBoundLeft:       {:>13.2f}\n", i, m_displays[i].rotationBoundLeft));
		LogToWix(fmt::format("MON {} rotationBoundRight:      {:>13.2f}\n", i, m_displays[i].rotationBoundRight));
		LogToWix(fmt::format("MON {} rotationBoundTop:        {:>13.2f}\n", i, m_displays[i].rotationBoundTop));
		LogToWix(fmt::format("MON {} rotationBoundBottom:     {:>13.2f}\n", i, m_displays[i].rotationBoundBottom));
	}

	LogToWix(fmt::format("\n{:-^}\n", "???"));

	return;
}

inline void SendMyInput(float x, float y)
{
	static MOUSEINPUT mi = {
		0,
		0,
		0,
		MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK,
		0,
		0
	};

	static INPUT ip = {
		INPUT_MOUSE,
		mi
	};

	ip.mi.dx = static_cast<LONG>(x);
	ip.mi.dy = static_cast<LONG>(y);

	SendInput(1, &ip, sizeof(INPUT));

	return;
}

void CTracker::MouseMove(int monitorCount, float yaw, float pitch)
{

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
	// The return statement is never reached if the head is pointing outside the bounds of any of the screens

	for (int i = 0; i <= monitorCount - 1; i++)
	{
		if ((yaw > m_displays[i].rotationBound16BitLeft)
			&& (yaw < m_displays[i].rotationBound16BitRight)
			&& (pitch < m_displays[i].rotationBound16BitTop)
			&& (pitch > m_displays[i].rotationBound16BitBottom))
		{
			// I wrote it out for maintainability
			// its plenty fast anyway for a 60hz limited display
			rl = m_displays[i].rotationBound16BitLeft;
			al = m_displays[i].boundAbsLeft;
			mx = m_displays[i].xSlope;
			x  = mx * (yaw - rl) + al;
			rt = m_displays[i].rotationBound16BitTop;
			at = m_displays[i].boundAbsTop;
			my = m_displays[i].ySlope;
			y  = my * (rt - pitch) + at;
			// load the coordinates into my input structure
			// need to cast to an integer because resulting calcs are floats
			SendMyInput(x, y);
			lastScreen = i;
			//LogToWix(fmt::format("(%f, %f)", y, x); // for testing
			return;
		}
	}

	// If the head is pointing outside of the bounds of a screen the mouse
	// should snap to the breached edge it could either be the pitch or the
	// yaw axis that is too great or too little to do this assume the pointer
	// came from the last screen, just asign the mouse position to the absolute
	// limit from the screen it came from.
	if (yaw < m_displays[lastScreen].rotationBound16BitLeft)
	{
		x = m_displays[lastScreen].boundAbsLeft
			+ m_displays[lastScreen].paddingLeft * m_xPixelAbsoluteSlope;
	}
	else if (yaw > m_displays[lastScreen].rotationBound16BitRight)
	{
		x = m_displays[lastScreen].boundAbsRight
			- m_displays[lastScreen].paddingRight * m_xPixelAbsoluteSlope;
	}
	else
	{
		rl = m_displays[lastScreen].rotationBound16BitLeft;
		al = m_displays[lastScreen].boundAbsLeft;
		mx = m_displays[lastScreen].xSlope;
		x  = mx * (yaw - rl) + al;
	}

	if (pitch > m_displays[lastScreen].rotationBound16BitTop)
	{
		y = m_displays[lastScreen].boundAbsTop
			+ m_displays[lastScreen].paddingTop * m_yPixelAbsoluteSlope;
	}
	else if (pitch < m_displays[lastScreen].rotationBound16BitBottom)
	{
		y = m_displays[lastScreen].boundAbsBottom
			- m_displays[lastScreen].paddingBottom * m_yPixelAbsoluteSlope;
	}
	else
	{
		rt = m_displays[lastScreen].rotationBound16BitTop;
		at = m_displays[lastScreen].boundAbsTop;
		my = m_displays[lastScreen].ySlope;
		y  = my * (rt - pitch) + at;
	}

	SendMyInput(x, y);

	return;
}