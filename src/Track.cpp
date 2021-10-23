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

#define FMT_HEADER_ONLY
#include <fmt\format.h>
#include <fmt\xchar.h>
#include <wx/wx.h>

#define USHORT_MAX_VAL 65535 // SendInput with absolute mouse movement takes a short int

bool g_bPauseTracking = false;

// Uncomment this line for testing to prevent program
// from attaching to NPTrackIR and supersede control
//#define TEST_NO_TRACK

//void disconnectTrackIR(void);


CTracker::CTracker(wxEvtHandler* m_parent, HWND hWnd, CConfig* config)
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

	WinSetup();
	
	DisplaySetup(config);

	// After settings are loaded, start accepting connections & msgs
	// on a named pipe to externally controll the track IR process
	// Start the watchdog thread
	if (config->m_bWatchdog)
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
	std::wstring wide_string = convert.from_bytes(config -> m_sTrackIrDllLocation);
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
		LogToWix(fmt::format("\nFAILED TO LOAD TRACKIR DLL: {}\n\n", rslt));
		throw Exception("Failed to load track IR DLL.");
	}
	else
	{
		LogToWix(fmt::format("\nNP INITIALIZATION FAILED WITH CODE: {}\n\n", rslt));
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
		LogToWix(fmt::format("\nBOOTING CONTROL OF PREVIOUS MOUSETRACKER INSTANCE!\n\n"));
		Sleep(2);
		rslt = NP_RegisterWindowHandle(hConsole);
	}

	if (NP_OK == rslt)
	{
		LogToWix("Register Window Handle:      Success\n");
	}
	else
	{
		LogToWix(fmt::format("Register Window Handle: Failed {:>3}\n", rslt));
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
		LogToWix(fmt::format("Request Data:        Failed: {:>3}\n", rslt));
		throw Exception("");
	}

	rslt = NP_RegisterProgramProfileID(config->m_profileID);
	if (NP_OK == rslt)
	{
		LogToWix("Register Profile ID:         Success\n");
	}
	else
	{
		LogToWix(fmt::format("Register Profile ID:       Failed: {:>3}\n", rslt));
		throw Exception("");
	}

#endif
}

int CTracker::trackStart(CConfig* config)
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
		LogToWix(fmt::format("Start Data Transmission:     Failed: {:>3}\n", rslt));
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
				MouseMove(config->m_monitorCount, yaw, pitch);
			}

			// If I would like to log rotational data
			//LogToWix(fmt::format("fNPYaw: {:f}\n", yaw));
			//LogToWix(fmt::format("fNPPitch: {:f}\n", pitch));

			// I don't actually really care about dropped frames
			//if ((framesig != lastFrame + 1) && (lastFrame != 0))
			//{
				//LogToWix(fmt::format("dropped"));
				//droppedFrames = droppedFrames + framesig - lastFrame - 1;
			//}

			lastFrame = framesig;
		}
		else if (NP_ERR_DEVICE_NOT_PRESENT == gdf)
		{
			LogToWix(fmt::format("\n\nDEVICE NOT PRESENT\nSTOPPING TRACKING...\nPLEASE RESTART PROGRAM\n\n"));
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

	m_displays[count].pixelBoundLeft = static_cast <unsigned int> (Monitor.rcMonitor.left); // from long
	m_displays[count].pixelBoundRight = static_cast <unsigned int> (Monitor.rcMonitor.right); // from long
	m_displays[count].pixelBoundTop = static_cast <unsigned int> (Monitor.rcMonitor.top); // from long
	m_displays[count].pixelBoundBottom = static_cast <unsigned int> (Monitor.rcMonitor.bottom); // from long

	// Remember to not use {s} for string types. This causes an error for some reason.
	LogToWix(fmt::format(L"MON Name:{:>15}\n", Monitor.szDevice));

	LogToWix(fmt::format("MON {} Left:   {:>10}\n", count, m_displays[count].pixelBoundLeft));
	LogToWix(fmt::format("MON {} Right:  {:>10}\n", count, m_displays[count].pixelBoundRight));
	LogToWix(fmt::format("MON {} Top:    {:>10}\n", count, m_displays[count].pixelBoundTop));
	LogToWix(fmt::format("MON {} Bottom: {:>10}\n", count, m_displays[count].pixelBoundBottom));

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

void CTracker::WinSetup()
{
	LogToWix(fmt::format("\n{:-^50}\n", "Windows Environment Info"));

	int monitorCount = GetSystemMetrics(SM_CMONITORS);
	int virtualDesktopWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);  // width of total bounds of all screens
	int virtualDesktopHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN); // height of total bounds of all screens

	LogToWix(fmt::format("{} Monitors Found\n", GetSystemMetrics(SM_CMONITORS)));
	LogToWix(fmt::format("Width of Virtual Desktop:  {:>5}\n", virtualDesktopWidth));
	LogToWix(fmt::format("Height of Virtual Desktop: {:>5}\n", virtualDesktopHeight));

	// TODO: move this to config or turn my array into a vector
	if (DEFAULT_MAX_DISPLAYS < monitorCount)
	{
		LogToWix(fmt::format("More Than {} Displays Found.\nIncrease max number of m_displays.\n", DEFAULT_MAX_DISPLAYS));
	}

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

	LogToWix(fmt::format("\nVirtual Desktop Pixel Bounds\n"));
	EnumDisplayMonitors(NULL, NULL, WrapperPopulateVirtMonitorBounds, reinterpret_cast<LPARAM>(this));

	LogToWix(fmt::format("\nVirtual Origin Offset Horizontal: {:d}\n", m_virtualOriginX));
	LogToWix(fmt::format("Virtual Origin Offset Vertical:   {:d}\n", m_virtualOriginY));

	return;
}

void CTracker::DisplaySetup(CConfig* config)
{
	for (int i = 0; i < config->m_monitorCount; i++)
	{
		m_displays[i].rotationBoundLeft   = config->bounds[i].left;
		m_displays[i].rotationBoundRight  = config->bounds[i].right;
		m_displays[i].rotationBoundTop    = config->bounds[i].top;
		m_displays[i].rotationBoundBottom = config->bounds[i].bottom;

		m_displays[i].paddingLeft         = config->bounds[i].paddingLeft;
		m_displays[i].paddingRight        = config->bounds[i].paddingRight;
		m_displays[i].paddingTop          = config->bounds[i].paddingTop;
		m_displays[i].paddingBottom       = config->bounds[i].paddingBottom;

		m_displays[i].setAbsBounds(m_virtualOriginX, m_virtualOriginY, m_xPixelAbsoluteSlope, m_yPixelAbsoluteSlope);
	}
	LogToWix(fmt::format("\nVirtual Desktop Pixel Bounds (abs)\n"));
	for (int i = 0; i < config->m_monitorCount; i++)
	{
		LogToWix(fmt::format("MON {} pixelBoundboundAbsLeft:   {:>10d}\n", i, m_displays[i].pixelBoundAbsLeft));
		LogToWix(fmt::format("MON {} pixelBoundboundAbsRight:  {:>10d}\n", i, m_displays[i].pixelBoundAbsRight));
		LogToWix(fmt::format("MON {} pixelBoundboundAbsTop:    {:>10d}\n", i, m_displays[i].pixelBoundAbsTop));
		LogToWix(fmt::format("MON {} pixelBoundboundAbsBottom: {:>10d}\n", i, m_displays[i].pixelBoundAbsBottom));
	}
	LogToWix(fmt::format("\n16-bit Coordinate Bounds\n"));
	for (int i = 0; i < config->m_monitorCount; i++)
	{
		LogToWix(fmt::format("MON {} boundAbsLeft:       {:>12.1f}\n", i, m_displays[i].boundAbsLeft));
		LogToWix(fmt::format("MON {} boundAbsRight:      {:>12.1f}\n", i, m_displays[i].boundAbsRight));
		LogToWix(fmt::format("MON {} boundAbsTop:        {:>12.1f}\n", i, m_displays[i].boundAbsTop));
		LogToWix(fmt::format("MON {} boundAbsBottom:     {:>12.1f}\n", i, m_displays[i].boundAbsBottom));
	}
	LogToWix(fmt::format("\nRotational Bounds\n"));
	for (int i = 0; i < config->m_monitorCount; i++)
	{
		LogToWix(fmt::format("MON {} rotationBoundLeft:       {:>13.2f}\n", i, m_displays[i].rotationBoundLeft));
		LogToWix(fmt::format("MON {} rotationBoundRight:      {:>13.2f}\n", i, m_displays[i].rotationBoundRight));
		LogToWix(fmt::format("MON {} rotationBoundTop:        {:>13.2f}\n", i, m_displays[i].rotationBoundTop));
		LogToWix(fmt::format("MON {} rotationBoundBottom:     {:>13.2f}\n", i, m_displays[i].rotationBoundBottom));
	}

	LogToWix(fmt::format("\n{:-^}\n", ""));

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
		if ((yaw > m_displays[i].rotationBound16BitLeft) && (yaw < m_displays[i].rotationBound16BitRight) && (pitch < m_displays[i].rotationBound16BitTop) && (pitch > m_displays[i].rotationBound16BitBottom))
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

	// If the head is pointing outside of the bounds of a screen the mouse should snap to the breached edge
	// It could either be the pitch or the yaw axis that is too great or too little
	// To do this assume the pointer came from the last screen, just asign the mouse position to the absolute limit from the screen it came from
	if (yaw < m_displays[lastScreen].rotationBound16BitLeft)
	{
		x = m_displays[lastScreen].boundAbsLeft + m_displays[lastScreen].paddingLeft * m_xPixelAbsoluteSlope;
	}
	else if (yaw > m_displays[lastScreen].rotationBound16BitRight)
	{
		x = m_displays[lastScreen].boundAbsRight - m_displays[lastScreen].paddingRight * m_xPixelAbsoluteSlope;
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
		y = m_displays[lastScreen].boundAbsTop + m_displays[lastScreen].paddingTop * m_yPixelAbsoluteSlope;
	}
	else if (pitch < m_displays[lastScreen].rotationBound16BitBottom)
	{
		y = m_displays[lastScreen].boundAbsBottom - m_displays[lastScreen].paddingBottom * m_yPixelAbsoluteSlope;
	}
	else
	{
		rt = m_displays[lastScreen].rotationBound16BitTop;
		at = m_displays[lastScreen].boundAbsTop;
		my = m_displays[lastScreen].ySlope;
		y  = my * (rt - pitch) + at;
	}

	// LogToWix(fmt::format("off monitors"));
	SendMyInput(x, y);

	return;

}

/*
// TODO: Future function to implement
int getDllLocationFromRegistry()
{
	TCHAR szPath[MAX_PATH * 2];
	HKEY pKey = NULL;
	LPTSTR pszValue;
	DWORD dwSize;
	if (::RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\NaturalPoint\\NATURALPOINT\\NPClient Location"), 0, KEY_READ, &pKey) != ERROR_SUCCESS)
	{
		//MessageBox(hWnd, _T("DLL Location key not present"), _T("TrackIR Client"), MB_OK);
		LogToWix(fmt::format("Registry Error: DLL Location key not present"));
		return false;
	}
	if (RegQueryValueEx(pKey, _T("Path"), NULL, NULL, NULL, &dwSize) != ERROR_SUCCESS)
	{
		//MessageBox(hWnd, _T("Path value not present"), _T("TrackIR Client"), MB_OK);
		LogToWix(fmt::format("Registry Error: Path value not present"));
		return false;
	}
	pszValue = (LPTSTR)malloc(dwSize);
	if (pszValue == NULL)
	{
		//MessageBox(hWnd, _T("Insufficient memory"), _T("TrackIR Client"), MB_OK);
		LogToWix(fmt::format("Registry Error: Insufficient memory"));
		return false;
	}
	if (RegQueryValueEx(pKey, _T("Path"), NULL, NULL, (LPBYTE)pszValue, &dwSize) != ERROR_SUCCESS)
	{
		::RegCloseKey(pKey);
		//MessageBox(hWnd, _T("Error reading location key"), _T("TrackIR Client"), MB_OK);
		LogToWix(fmt::format("Registry Error: Error reading location key"));
		return false;
	}
	else
	{
		::RegCloseKey(pKey);
		_tcscpy_s(szPath, pszValue);
		free(pszValue);
	}
}*/

/*

TODO:
make toml more robust to non floats

*/