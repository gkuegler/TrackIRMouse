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

#include <stdio.h>
#include <string.h>
#include <locale>
#include <TCHAR.H>
#include <format>

#include "Constants.h"
#include "NPClient.h"
#include "Core.h"
#include "Display.h"
#include "Config.h"
#include "Watchdog.h"


// Uncomment this line for testing to prevent program
// from attaching to NPTrackIR and supersede control
//#define TEST_NO_TRACK

//void pressAnyKeyToExit(void);
void disconnectTrackIR(void);
//HWND getCurrentConsoleHwnd(void);
int trackStart(wxEvtHandler* m_parent);
void logToWix(std::string msg);

//int main(int argc, char* argv[])
int trackStart(wxEvtHandler* m_parent, HWND hWnd)
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


	int iMonitorCount = WinSetup();

	CConfig Config;

	try
	{
		Config.LoadSettings(iMonitorCount);
	}
	catch (std::runtime_error e)
	{
		printf("Load Settings Failed. See TOML error above.");
		pressAnyKeyToExit();
	}
	
	DisplaySetup(iMonitorCount, Config);

	// After settings are loaded, start accepting connections & msgs
	// on a named pipe to kill the trackir process
	// Start the watchdog thread
	if (Config.bWatchdog)
	{
		HANDLE hThread = WatchDog::WD_StartWatchdog();
	}

	printf("\n--------TrackIR Iinit Status--------------\n");

	// TCHAR sDll[MAX_PATH] = L"C:\\Program Files (x86)\\NaturalPoint\\TrackIR5\\NPClient.dll";
	TCHAR sDll[MAX_PATH];

	std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> convert;
	std::wstring wide_string = convert.from_bytes(Config.sTrackIR_dll_location);
	const wchar_t* temp_wide_string = wide_string.c_str();
	wcscpy_s(sDll, MAX_PATH, temp_wide_string);

	// Load trackir dll and resolve function addresses
	NPRESULT rslt = NP_OK;
	rslt = NPClient_Init(sDll);
	
	if (NP_OK == rslt)
	{
		printf("NP Initialization Code: %d\n", rslt);
		std::atexit(disconnectTrackIR); // this isn't really doing anything at the moment
	}
	else
	{
		printf("NP Initialization Failed With Code: %d\n", rslt);
		pressAnyKeyToExit();
		return 1;
	}

#ifdef TEST_NO_TRACK
	// In the program early after the DLL is loaded for testing
	// so that this program doesn't boot control
	// from my local copy.
	pressAnyKeyToExit();
#endif
	
	// NP needs a window handle to send data frames to
	HWND hConsole = getCurrentConsoleHwnd();
	
	// NP_RegisterWindowHandle
	rslt = NP_OK;
	rslt = NP_RegisterWindowHandle(hConsole);

	if (rslt == 7)
	{
		NP_UnregisterWindowHandle();
		printf("! Booted Control of Previous MouseTracker Instance\n");
		Sleep(2);
		rslt = NP_RegisterWindowHandle(hConsole);
	}
	printf("Register Window Handle: %d\n", rslt);
	
	// I'm skipping query the software version, I don't think its necessary
	
	// NP_RequestData
	// Request roll, pitch. See NPClient.h
	unsigned short data_fields = NPPitch | NPYaw;
	rslt = NP_RequestData(data_fields); 
	printf("Request Data: %d\n", rslt);

	// NP_RegisterProgramProfileID: 13302
	rslt = NP_RegisterProgramProfileID(Config.profile_ID);
	printf("Register Program Profile ID: %d\n", rslt);

	// Skipping this too. I think this is for legacy games
	// NP_StopCursor

	// NP_StartDataTransmission
	rslt = NP_StartDataTransmission();
	printf("Start Data Transmission: %d\n", rslt);

	NPRESULT gdf;
	tagTrackIRData *pTIRData, TIRData;
	pTIRData = &TIRData;

	unsigned short last_frame = 0;
	int dropped_frames = 0;
	// bool connected_to_host = true;

	printf("---------------------\n");
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
				// printf("Tracking Paused\n");
				// TrackIR5 supposedly operates at 120hz
				// 8ms is approximately 1 frame
				Sleep(8);
				continue;
			}

			MouseMove(iMonitorCount, yaw, pitch);
			//printf("fNPYaw: %f\n", yaw);
			//printf("fNPPitch: %f\n", pitch);

			//if ((framesig != last_frame + 1) && (last_frame != 0))
			//{
				//printf("drop");
				//dropped_frames = dropped_frames + framesig - last_frame - 1;
			//}

			last_frame = framesig;
		}
		else if (NP_ERR_DEVICE_NOT_PRESENT == gdf)
		{
			printf("DEVICE NOT PRESENT");
			break;
		}

		//Sleep(8);
		//Sleep(1000); // Test case
	}
	
	return 0;
}

void pressAnyKeyToExit()
{
	char szLine[51]; // room for 20 chars + '\0'
	printf("\n\nPress Enter Key to Exit -> ");
	gets_s(szLine, 50);
}

void disconnectTrackIR()
{
	NP_UnregisterWindowHandle();
}

HWND getCurrentConsoleHwnd(void)
// Sets the Consol title to a known value
// Uses the title to FindWindow
// I guess Microsoft is phasing out GetConsoleWindow
{
	HWND HwndFound;
	TCHAR szVersionNumber[13] = VERSION; // add version number to window title
	TCHAR szNewWindowTitle[CONSOL_TITLE_BUFF] = L"MouseTrackIR ";

	wcscat_s(szNewWindowTitle, szVersionNumber);
	printf("Title: %ls\n", szNewWindowTitle);

	// Change current window title
	SetConsoleTitle(szNewWindowTitle);

	// Ensure window title has been updated.
	Sleep(40);

	// Look for NewWindowTitle
	HwndFound = FindWindow(NULL, szNewWindowTitle);

	if (HwndFound == NULL)
	{
		printf("Window Handle Not Found!\n");
	}

	return(HwndFound);
}

int getDllLocationFromRegistry()
{
	TCHAR szPath[MAX_PATH * 2];
	HKEY pKey = NULL;
	LPTSTR pszValue;
	DWORD dwSize;
	if (::RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\NaturalPoint\\NATURALPOINT\\NPClient Location"), 0, KEY_READ, &pKey) != ERROR_SUCCESS)
	{
		//MessageBox(hWnd, _T("DLL Location key not present"), _T("TrackIR Client"), MB_OK);
		printf("Registry Error: DLL Location key not present");
		return false;
	}
	if (RegQueryValueEx(pKey, _T("Path"), NULL, NULL, NULL, &dwSize) != ERROR_SUCCESS)
	{
		//MessageBox(hWnd, _T("Path value not present"), _T("TrackIR Client"), MB_OK);
		printf("Registry Error: Path value not present");
		return false;
	}
	pszValue = (LPTSTR)malloc(dwSize);
	if (pszValue == NULL)
	{
		//MessageBox(hWnd, _T("Insufficient memory"), _T("TrackIR Client"), MB_OK);
		printf("Registry Error: Insufficient memory");
		return false;
	}
	if (RegQueryValueEx(pKey, _T("Path"), NULL, NULL, (LPBYTE)pszValue, &dwSize) != ERROR_SUCCESS)
	{
		::RegCloseKey(pKey);
		//MessageBox(hWnd, _T("Error reading location key"), _T("TrackIR Client"), MB_OK);
		printf("Registry Error: Error reading location key");
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