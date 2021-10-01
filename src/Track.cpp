/*
* The release version is designed to run as an administrator
* UAC Execution Level /level=requireAdministrator
* The debug version leaves UAC alone.
* 
* I'm building to x86 primarily because I'm stuck using a
* 32-bit version of Python.
*/

#pragma warning(disable : 4996)
#include <wx/wx.h>

#define FMT_HEADER_ONLY
#include <fmt\format.h>

#include <stdio.h>
#include <string.h>
#include <locale>
#include <TCHAR.H>

#include "Constants.h"
#include "NPClient.h"
#include "Core.h"
#include "Display.h"
#include "Config.h"
#include "Watchdog.h"
#include "Log.h"
#include "Track.h"

//CConfig g_config;

// Uncomment this line for testing to prevent program
// from attaching to NPTrackIR and supersede control
//#define TEST_NO_TRACK

void disconnectTrackIR(void);

namespace Track {
int trackInitialize(wxEvtHandler* m_parent, HWND hWnd, CConfig* config)
{

	// ## Program flow ##
	// 
	// first thinrg is ping windows for info
	// load settings
	// validate settings
	// start watchdog thread
	// load trackir dll and getproc addresses
	// connect to trackir and request data type
	// receive data frames
	// decode data
	// hand data off to track program
	// calculate mouse position
	// form & send mouse move command


	logToWix(fmt::format("Starting Initialization Of TrackIR\n"));

	//CConfig Config;
	//Using a global variable instead of classes.
	//g_config = CConfig();

	int iMonitorCount = WinSetup();
	(config) -> iMonitorCount = iMonitorCount;


	try
	{
		(config)->LoadSettings(iMonitorCount);
	}
	catch (std::runtime_error e)
	{
		//logToWix(fmt::format("Load Settings Failed. See TOML error above."));
	}

	
	DisplaySetup(iMonitorCount, (config));

	// After settings are loaded, start accepting connections & msgs
	// on a named pipe to kill the trackir process
	// Start the watchdog thread
	if ((config)->bWatchdog)
	{
		HANDLE hThread = WatchDog::WD_StartWatchdog();
	}

	logToWix(fmt::format("\n{:-^50}\n", "TrackIR Iinit Status"));

	// Find and load TrackIR DLL
	TCHAR sDll[MAX_PATH];
	std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> convert;
	std::wstring wide_string = convert.from_bytes((config) -> sTrackIR_dll_location);
	const wchar_t* temp_wide_string = wide_string.c_str();
	wcscpy_s(sDll, MAX_PATH, temp_wide_string);

	// Load trackir dll and resolve function addresses
	NPRESULT rslt = NP_OK;
	rslt = NPClient_Init(sDll);

	if (NP_OK == rslt)
	{
		logToWix(fmt::format("NP Initialization Code:      {:>3}\n", rslt));
	}
	else
	{
		logToWix(fmt::format("\nNP INITIALIZATION FAILED WITH CODE: {}\n\n", rslt));
		return 1;
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

	if (rslt == 7)
	{
		NP_UnregisterWindowHandle();
		logToWix(fmt::format("\nBOOTED CONTROL OF PREVIOUS MOUSETRACKER INSTANCE!\n\n"));
		Sleep(2);
		rslt = NP_RegisterWindowHandle(hConsole);
	}

	logToWix(fmt::format("Register Window Handle:      {:>3}\n", rslt));

	// I'm skipping query the software version, I don't think its necessary

	// Request roll, pitch. See NPClient.h
	unsigned short data_fields = NPPitch | NPYaw;
	rslt = NP_RequestData(data_fields);
	logToWix(fmt::format("Request Data:                {:>3}\n", rslt));

#endif

	return 1;
}

int trackStart(CConfig* config)
{
	NPRESULT rslt = NP_RegisterProgramProfileID((config)->profile_ID);
	logToWix(fmt::format("Register Program Profile ID: {:>3}\n", rslt));

	// Skipping this too. I think this is for legacy games
	// NP_StopCursor

	rslt = NP_StartDataTransmission();
	logToWix(fmt::format("Start Data Transmission:     {:>3}\n", rslt));

	NPRESULT gdf;
	tagTrackIRData* pTIRData, TIRData;
	pTIRData = &TIRData;

	unsigned short last_frame = 0;
	int dropped_frames = 0;

	while(true)
	{
		gdf = NP_GetData(pTIRData);
		if (NP_OK == gdf)
		{
			unsigned short status = (*pTIRData).wNPStatus;
			unsigned short framesig = (*pTIRData).wPFrameSignature;
			float yaw = -(*pTIRData).fNPYaw; // Make Future optimization to not need a negative sign
			float pitch = -(*pTIRData).fNPPitch;

			// Don't try to move the mouse when TrackIR is paused
			if (framesig == last_frame)
			{
				// logToWix(fmt::format("Tracking Paused\n");
				// TrackIR5 supposedly operates at 120hz
				// 8ms is approximately 1 frame
				Sleep(8);
				continue;
			}

			MouseMove((config)->iMonitorCount, yaw, pitch);

			// If I would like to log rotational data
			//logToWix(fmt::format("fNPYaw: {:f}\n", yaw));
			//logToWix(fmt::format("fNPPitch: {:f}\n", pitch));

			// I don't actually really care about dropped frames
			//if ((framesig != last_frame + 1) && (last_frame != 0))
			//{
				//logToWix(fmt::format("dropped"));
				//dropped_frames = dropped_frames + framesig - last_frame - 1;
			//}

			last_frame = framesig;
		}
		else if (NP_ERR_DEVICE_NOT_PRESENT == gdf)
		{
			logToWix(fmt::format("\n\nDEVICE NOT PRESENT\nSTOPPING TRACKING...\nPLEASE RESTART PROGRAM\n\n"));
			break;
		}

		Sleep(8);
	}
	
	return 0;
}

void trackStop()
{
	NP_StopDataTransmission();
	NP_UnregisterWindowHandle();
}

}


// Future function to implement
int getDllLocationFromRegistry()
{
	TCHAR szPath[MAX_PATH * 2];
	HKEY pKey = NULL;
	LPTSTR pszValue;
	DWORD dwSize;
	if (::RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\NaturalPoint\\NATURALPOINT\\NPClient Location"), 0, KEY_READ, &pKey) != ERROR_SUCCESS)
	{
		//MessageBox(hWnd, _T("DLL Location key not present"), _T("TrackIR Client"), MB_OK);
		logToWix(fmt::format("Registry Error: DLL Location key not present"));
		return false;
	}
	if (RegQueryValueEx(pKey, _T("Path"), NULL, NULL, NULL, &dwSize) != ERROR_SUCCESS)
	{
		//MessageBox(hWnd, _T("Path value not present"), _T("TrackIR Client"), MB_OK);
		logToWix(fmt::format("Registry Error: Path value not present"));
		return false;
	}
	pszValue = (LPTSTR)malloc(dwSize);
	if (pszValue == NULL)
	{
		//MessageBox(hWnd, _T("Insufficient memory"), _T("TrackIR Client"), MB_OK);
		logToWix(fmt::format("Registry Error: Insufficient memory"));
		return false;
	}
	if (RegQueryValueEx(pKey, _T("Path"), NULL, NULL, (LPBYTE)pszValue, &dwSize) != ERROR_SUCCESS)
	{
		::RegCloseKey(pKey);
		//MessageBox(hWnd, _T("Error reading location key"), _T("TrackIR Client"), MB_OK);
		logToWix(fmt::format("Registry Error: Error reading location key"));
		return false;
	}
	else
	{
		::RegCloseKey(pKey);
		_tcscpy_s(szPath, pszValue);
		free(pszValue);
	}
}

/*

TODO:
create class cpp/h for display
maybe create additional headers for NP stuff
use include gaurds? #ifdef
delete debug .cpp files
make toml more robust to non floats
move toml load to seperate file
make uniform standard for variables and functions for cases
variables prefixed and Camel

*/