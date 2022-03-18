#pragma once

#include "..\..\src\np-client.h"

char gpTextBuf[16384];  // 16k text buffer
unsigned long NPFrameSignature;
unsigned long NPStaleFrames;

//////////////////////////
// Function Prototypes //////////////////////////////////////////////////////
/////////////////////////
//
NPRESULT client_HandleTrackIRData();

// The following timer is used to simulate the per-frame update of an
// gaming application, where it will check for new Tracking Data.

// Start the timer routine, and set the call interval at 17 milliseconds
// (~ 60 times per second). The TrackIR hardware can actually update
// at up to 120 times per second, though.
// SetTimer(1, 17, NULL);

//
// Timer routine for test purposes -- simply pumps numbered text messages to the
// output window to check UI message scrolling, etc.
//
// void CNPTestDlg::OnTimer(UINT_PTR nIDEvent)
//{
//	NPRESULT result;
//	CString csTimerMsg;
//
//	m_nTimerMessageNum++;
//
//	// "Poll" the NPClient interface for new TrackIR data, and process it if
// found 	result = client_HandleTrackIRData();
//
//	if (result == NP_ERR_NO_DATA)
//	{
////		csTimerMsg.Format( "No new data on timer call %d",
/// m_nTimerMessageNum); /		DisplayLine( csTimerMsg );
//	}
//
//
//	CDialog::OnTimer(nIDEvent);
//
//} // CNPTestDlg::OnTimer()

// ************************************************************
// ***     TrackIR Enhanced SDK Initialization procedure    ***
// ***    This implementation uses the DLL wrapper module   ***
// ************************************************************
void TrackIR_Enhanced_Init() {
  NPRESULT result;

  // Zero TrackIR SDK Related counters
  NPFrameSignature = 0;
  NPStaleFrames = 0;

  // Locate the TrackIR Enhanced DLL
  TCHAR szPath[MAX_PATH * 2];

  // Initialize the the TrackIR Enhanced DLL
  result = NPClient_Init(szPath);
  if (NP_OK == result)
    DisplayLine("NPClient interface -- initialize OK.");
  else
    DisplayLine("Error initializing NPClient interface!!");

  // Register your applications Window Handle
  result = NP_RegisterWindowHandle(GetSafeHwnd());
  if (NP_OK == result)
    DisplayLine("NPClient : Window handle registration successful.");
  else
    DisplayLine("NPClient : Error registering window handle!!");

  // Query for the NaturalPoint TrackIR software versione
  unsigned short wNPClientVer;
  result = NP_QueryVersion(&wNPClientVer);
  if (NP_OK == result) {
    CString csMajorVer, csMinorVer, csVerMsg;
    csMajorVer.Format("%d", (wNPClientVer >> 8));
    csMinorVer.Format("%02d", (wNPClientVer & 0x00FF));
    csVerMsg.Format("NaturalPoint software version is %s.%s", csMajorVer,
                    csMinorVer);
    DisplayLine(csVerMsg);
  } else
    DisplayLine("NPClient : Error querying NaturalPoint software version!!");

  // Choose the Axes that you want tracking data for
  unsigned int DataFields = 0;

  // Rotation Axes
  DataFields |= NPPitch;
  DataFields |= NPYaw;
  DataFields |= NPRoll;

  // Translation Axes
  DataFields |= NPX;
  DataFields |= NPY;
  DataFields |= NPZ;

  // Register the Axes selection with the TrackIR Enhanced interface
  NP_RequestData(DataFields);

  // It is *required* that your application registers the Developer ID
  // assigned by NaturalPoint!

  // Your assigned developer ID needs to be inserted below. Use ID: 1000 if no
  // specific ID was assigned. #define NP_DEVELOPER_ID YOUR-NUMBER-GOES-HERE

  // NOTE : The title of your project must show up
  // in the list of supported titles shown in the Profiles
  // tab of the TrackIR software, if it does not then the
  // TrackIR software will *not* transmit data to your
  // application. If your title is not present in the list,
  // you may need to have the TrackIR software perform a
  // game list update (to download support for new Developer IDs)
  // using the menu item under the "Help" or "Update" menu.

  NP_RegisterProgramProfileID(NP_DEVELOPER_ID);

  // Stop the cursor
  result = NP_StopCursor();
  if (result == NP_OK)
    DisplayLine("Cursor stopped");
  else
    DisplayLine("NPCient : Error stopping cursor");

  // Request that the TrackIR software begins sending Tracking Data
  result = NP_StartDataTransmission();
  if (result == NP_OK)
    DisplayLine("Data transmission started");
  else
    DisplayLine("NPCient : Error starting data transmission");
}

// ************************************************************
// ***       TrackIR Enhanced SDK Shutdown procedure        ***
// ************************************************************

void CNPTestDlg::TrackIR_Enhanced_Shutdown() {
  // Request that the TrackIR software stop sending Tracking Data
  NP_StopDataTransmission();

  // Un-register your applications Windows Handle
  NP_UnregisterWindowHandle();
}

// *************************************************************
// ***   TrackIR Enhanced SDK example new frame procedure    ***
// ***   (called by the timer function for this application) ***
// *************************************************************

NPRESULT client_HandleTrackIRData() {
  TRACKIRDATA tid;
  CString csDataRxMsg;
  CString t_str;

  // Query the TrackIR Enhanced Interface for the latest data
  NPRESULT result = NP_GetData(&tid);

  // If the call succeeded, then we have data to process
  if (NP_OK == result) {
    // Make sure the remote interface is active
    if (tid.wNPStatus == NPSTATUS_REMOTEACTIVE) {
      // Compare the last frame signature to the current one if
      // they are not the same then the data is new
      if (NPFrameSignature != tid.wPFrameSignature) {
        // In your own application, this is where you would utilize
        // the Tracking Data for View Control / etc.

        // Display the Tracking Data
        t_str.Format(
            "Rotation : NPPitch = %04.02f, NPYaw = %04.02f, NPRoll = %04.02f "
            "\r\nTranslation : NPX = %04.02f, NPY = %04.02f, NPZ = %04.02f "
            "\r\nInformation NPStatus = %d, Frame = %d",
            tid.fNPPitch, tid.fNPYaw, tid.fNPRoll, tid.fNPX, tid.fNPY, tid.fNPZ,
            tid.wNPStatus, tid.wPFrameSignature);
        pNPTestDlg->DisplayData(t_str);
        NPFrameSignature = tid.wPFrameSignature;
        NPStaleFrames = 0;

        //
        // All other data fields in TRACKIRDATA can be handled in a similar way.
        //
      } else {
        // Either there is no tracking data, the user has
        // paused the trackIR, or the call happened before
        // the TrackIR was able to update the interface
        // with new data

        if (NPStaleFrames > 30) {
          t_str.Format("No New Data. Paused or Not Tracking?", NPStaleFrames);
        } else {
          NPStaleFrames++;
          t_str.Format("No New Data for %d frames", NPStaleFrames);
        }
        result = NP_ERR_NO_DATA;
      }
    } else {
      // The user has set the device out of trackIR Enhanced Mode
      // and into Mouse Emulation mode with the hotkey
      t_str.Format("User Disabled");
      result = NP_ERR_NO_DATA;
    }
  }

  return result;
}
