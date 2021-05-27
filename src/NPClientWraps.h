// *******************************************************************************
// *
// * Module Name:
// *   NPClientWraps.h
// *
// * Software Engineer:
// *   Doyle Nickless - GoFlight Inc., for Eye Control Technology.
// *
// * Abstract:
// *   Header file for NPClientWraps.cpp module.
// *
// * Environment:
// *   User mode
// *
// *******************************************************************************
//


/////////////
// Defines ///////////////////////////////////////////////////////////////////////
/////////////
//

/////////////////////////
// Function Prototypes ///////////////////////////////////////////////////////////
/////////////////////////
//
NPRESULT __stdcall NP_RegisterWindowHandle(HWND hWnd);
NPRESULT __stdcall NP_UnregisterWindowHandle();
NPRESULT __stdcall NP_RegisterProgramProfileID(unsigned short wPPID);
NPRESULT __stdcall NP_QueryVersion(unsigned short* pwVersion);
NPRESULT __stdcall NP_RequestData(unsigned short wDataReq);
NPRESULT __stdcall NP_GetSignature(LPTRACKIRSIGNATURE pSignature);
NPRESULT __stdcall NP_GetData(LPTRACKIRDATA pTID);
NPRESULT __stdcall NP_StartCursor();
NPRESULT __stdcall NP_StopCursor();
NPRESULT __stdcall NP_ReCenter();
NPRESULT __stdcall NP_StartDataTransmission();
NPRESULT __stdcall NP_StopDataTransmission();

NPRESULT NPClient_Init(LPTSTR pszDLLPath);