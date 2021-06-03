/*
* The release version is designed to run as an administrator
* UAC Execution Level /level=requireAdministrator
* The debug version leaves UAC alone.
*/

#pragma warning(disable : 4996)

#include <stdio.h>
#include <string.h>
#include <TCHAR.H>

#define _AFXDLL
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>			// MFC support for Windows Common Controls

#include "Constants.h"
#include "NPClient.h"
#include "core.h"
#include "Display.h"
#include "Config.h"
#include "Watchdog.h"

void FnPressAnyKeyToExit(void);
void DisconnectTrackIR(void);
HWND GetCurrentConsoleHwnd(void);

int main(int argc, char* argv[])
{

	// ## Program flow ##
	// 
	// first thing is ping windows for info
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
		FnPressAnyKeyToExit();
	}
	
	DisplaySetup(iMonitorCount, Config);

	// After settings are loaded, start accepting connections & msgs
	// on a named pipe to kill the trackir process
	// Start the watchdog thread
	DWORD  dwThreadId = 0;
	HANDLE hThread = INVALID_HANDLE_VALUE;
	BOOL bEnableWatchdog = TRUE;


	if (bEnableWatchdog)
	{
		LPCSTR event_name = "WatchdogInitThread";

		// Create an event to signal, to ensure pipe initialization
		// occurs before continuing. This mostly matters so that the print statements
		// of the pipe initialization are not mixed in with the rest of the program
		HANDLE hEvent = CreateEventA(
			NULL,
			TRUE,
			0,
			event_name
		);

		hThread = CreateThread(
			NULL,              // no security attribute 
			0,                 // default stack size 
			WatchDog::WDInstanceThread,  // thread proc
			&hEvent,              // thread parameter
			0,                 // not suspended 
			&dwThreadId			// returns thread ID
		);      

		if (hThread == NULL)
		{
			printf("CreateThread failed, GLE=%d.\n", GetLastError());
			return 1;
		}

		printf("Thread Created Successfully\n");

		if (hEvent)
		{
			BOOL result = WaitForSingleObject(
				hEvent,
				3000		 // timeout in milliseconds
			);

		}
		

	}

	printf("\n--------TrackIR Iinit Status--------------\n");
	TCHAR sDll[MAX_PATH] = L"C:\\Program Files (x86)\\NaturalPoint\\TrackIR5";
	// Array decay into pointer?
	LPTSTR plsDLL = sDll;

	// Load trackir dll and resolve function addresses
	NPRESULT iRsltInit;
	iRsltInit = NPClient_Init(plsDLL);
	
	if (NP_OK == iRsltInit) {
		printf("NP Initialization Code: %d\n", iRsltInit);
		std::atexit(DisconnectTrackIR); // this isn't really doing anything at the moment
	}
	else {
		printf("NP Initialization Failed With Code: %d\n", iRsltInit);
		FnPressAnyKeyToExit();
		return 1;
	}
	
	// NP needs a window handle to send data frames to
	HWND hConsole = GetCurrentConsoleHwnd();
	
	// NP_RegisterWindowHandle
	NPRESULT iRsltNP = NP_OK;
	iRsltNP = NP_RegisterWindowHandle(hConsole);

	if (iRsltNP == 7) {
		NP_UnregisterWindowHandle();
		printf("! Booted Control of Previous MouseTracker Instance\n");
		Sleep(2);
		iRsltNP = NP_RegisterWindowHandle(hConsole);
	}
	printf("Register Window Handle: %d\n", iRsltNP);
	
	// I'm skipping query the software version, I don't think its necessary
	
	// NP_RequestData
	//unsigned int DataFields = 0;
	//DataFields |= NPPitch;
	//DataFields |= NPYaw;
	iRsltNP = NP_RequestData(119); // Request roll,pitch,yaw and x,y,z; See NPClient.h
	printf("Request Data: %d\n", iRsltNP);

	// NP_RegisterProgramProfileID: 13302
	iRsltNP = NP_RegisterProgramProfileID(Config.profile_ID);
	printf("Register Program Profile ID: %d\n", iRsltNP);
	
	// Skipping this too. I think this is for legacy games
	// NP_StopCursor

	// NP_StartDataTransmission
	iRsltNP = NP_StartDataTransmission();
	printf("Start Data Transmission: %d\n", iRsltNP);

	NPRESULT gdf;
	tagTrackIRData *pTIRData, TIRData;
	pTIRData = &TIRData;

	unsigned short last_frame = 0;
	int dropped_frames = 0;
	bool connected_to_host = true;

	printf("---------------------\n");
	while(true) {
		gdf = NP_GetData(pTIRData);
		if (NP_OK == gdf) {
			unsigned short status = (*pTIRData).wNPStatus;
			unsigned short framesig = (*pTIRData).wPFrameSignature;
			float yaw = -(*pTIRData).fNPYaw; // Future optimization to not need a negative sign
			float pitch = -(*pTIRData).fNPPitch;

			// Don't try to move the mouse when TrackIR is paused
			if (framesig == last_frame) {
				// printf("Tracking Paused\n");
				Sleep(8);
				continue;
			}

			MouseMove(iMonitorCount, yaw, pitch);
			//printf("fNPYaw: %f\n", yaw);
			//printf("fNPPitch: %f\n", pitch);

			//if ((framesig != last_frame + 1) && (last_frame != 0)) {
				//printf("drop");
				//dropped_frames = dropped_frames + framesig - last_frame - 1;
			//}

			last_frame = framesig;
		}
		else if (NP_ERR_DEVICE_NOT_PRESENT == gdf) {
			printf("DEVICE NOT PRESENT");
			break;
		}

		//Sleep(8);
		//Sleep(1000); // Test case
	}
	
	return 0;
}

void FnPressAnyKeyToExit()
{
	char line[51]; // room for 20 chars + '\0'
	printf("\n\nPress Enter Key to Exit -> ");
	gets_s(line, 50);
}

void DisconnectTrackIR()
{
	NP_UnregisterWindowHandle();
}

HWND GetCurrentConsoleHwnd(void)
// Sets the Consol title to a known value
// Uses the title to FindWindow
// I guess Microsoft is phasing out GetConsoleWindow
{
	HWND HwndFound;
	TCHAR version_no[13] = VERSION; // add version number to window title
	TCHAR PszNewWindowTitle[CONSOL_TITLE_BUFF] = L"MouseTrackIR ";

	wcscat_s(PszNewWindowTitle, version_no);
	printf("Title: %ls\n", PszNewWindowTitle);

	// Change current window title
	SetConsoleTitle(PszNewWindowTitle);

	// Ensure window title has been updated.
	Sleep(40);

	// Look for NewWindowTitle
	HwndFound = FindWindow(NULL, PszNewWindowTitle);

	if (HwndFound == NULL)
	{
		printf("Window Handle Not Found!\n");
	}

	return(HwndFound);
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