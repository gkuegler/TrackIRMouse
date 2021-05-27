// *******************************************************************************
// *
// * Module Name:
// *   NPClient.h
// *
// * Doyle Nickless -- 13 Jan, 2003 -- for Eye Control Technology.
// *
// * Abstract:
// *   Header for NaturalPoint Game Client API.
// *
// * Environment:
// *   Microsoft Windows -- User mode
// *
// *******************************************************************************

//#define DLL_NP_LOCATION "C:\\Program Files (x86)\\NaturalPoint\\TrackIR5"

#pragma pack( push, npclient_h ) // Save current pack value
#pragma pack(1)

//////////////////
/// Defines //////////////////////////////////////////////////////////////////////
/////////////////
#define         VERSION_MAJOR           1
#define         VERSION_MINOR           0
#define         VERSION_BUILD           1

// magic to get the preprocessor to do what we want
#define		lita(arg) #arg
#define		xlita(arg) lita(arg)
#define		cat3(w,x,z) w##.##x##.##z##\000
#define		xcat3(w,x,z) cat3(w,x,z)
#define		VERSION_STRING xlita(xcat3(VERSION_MAJOR,VERSION_MINOR,VERSION_BUILD))
//
// Versioning hasn't been worked out yet...
//
// The following is the previous spec definition of versioning info -- I can probably do
// something very similar to this -- will keep you posted.
//
// request version information using 2 messages, they cannot be expected to arrive in a specific order - so always parse using the High byte
// the messages have a NPCONTROL byte in the first parameter, and the second parameter has packed bytes.
//   Message 1) (first parameter)NPCONTROL : (second parameter) (High Byte)NPVERSIONMAJOR (Low Byte) major version number data
//   Message 2) (first parameter)NPCONTROL : (second parameter) (High Byte)NPVERSIONMINOR (Low Byte) minor version number data

#define	NPQUERYVERSION	1040

#define	NPSTATUS_REMOTEACTIVE	0
#define	NPSTATUS_REMOTEDISABLED	1

// CONTROL DATA SUBFIELDS
#define	NPVERSIONMAJOR	1
#define	NPVERSIONMINOR	2

// DATA FIELDS
#define	NPControl		8	// indicates a control data field
						// the second parameter of a message bearing control data information contains a packed data format. 
						// The High byte indicates what the data is, and the Low byte contains the actual data
// roll, pitch, yaw
#define	NPRoll		1	// +/- 16383 (representing +/- 180) [data = input - 16383] float
#define	NPPitch		2	// +/- 16383 (representing +/- 180) [data = input - 16383] float
#define	NPYaw		4	// +/- 16383 (representing +/- 180) [data = input - 16383] float

// x, y, z - remaining 6dof coordinates
#define	NPX			16	// +/- 16383 [data = input - 16383] float
#define	NPY			32	// +/- 16383 [data = input - 16383] float
#define	NPZ			64	// +/- 16383 [data = input - 16383] float

// raw object position from imager
#define	NPRawX		128	// 0..25600 (actual value is multiplied x 100 to pass two decimal places of precision)  [data = input / 100]
#define	NPRawY		256  // 0..25600 (actual value is multiplied x 100 to pass two decimal places of precision)  [data = input / 100]
#define	NPRawZ		512  // 0..25600 (actual value is multiplied x 100 to pass two decimal places of precision)  [data = input / 100]

// x, y, z deltas from raw imager position 
#define	NPDeltaX		1024 // +/- 2560 (actual value is multiplied x 10 to pass two decimal places of precision)  [data = (input / 10) - 256]
#define	NPDeltaY		2048 // +/- 2560 (actual value is multiplied x 10 to pass two decimal places of precision)  [data = (input / 10) - 256]
#define	NPDeltaZ		4096 // +/- 2560 (actual value is multiplied x 10 to pass two decimal places of precision)  [data = (input / 10) - 256]

// raw object position from imager
#define	NPSmoothX		8192	  // 0..32766 (actual value is multiplied x 10 to pass one decimal place of precision) [data = input / 10]
#define	NPSmoothY		16384  // 0..32766 (actual value is multiplied x 10 to pass one decimal place of precision) [data = input / 10]
#define	NPSmoothZ		32768  // 0..32766 (actual value is multiplied x 10 to pass one decimal place of precision) [data = input / 10]


//////////////////
/// Typedefs /////////////////////////////////////////////////////////////////////
/////////////////

// NPESULT values are returned from the Game Client API functions.
//
//enum class E_MY_FAVOURITE_FRUITS : unsigned char
typedef enum tagNPResult
{
	NP_OK = 0,
    NP_ERR_DEVICE_NOT_PRESENT,
	NP_ERR_UNSUPPORTED_OS,
	NP_ERR_INVALID_ARG,
	NP_ERR_DLL_NOT_FOUND,
	NP_ERR_NO_DATA,
	NP_ERR_INTERNAL_DATA,

} NPRESULT;

typedef struct tagTrackIRSignature {

	char DllSignature[200];
	char AppSignature[200];

} SIGNATUREDATA, *LPTRACKIRSIGNATURE;

typedef struct tagTrackIRData
{
	unsigned short wNPStatus;
	unsigned short wPFrameSignature;
	unsigned long  dwNPIOData;

	float fNPRoll;
	float fNPPitch;
	float fNPYaw;
	float fNPX;
	float fNPY;
	float fNPZ;
	float fNPRawX;
	float fNPRawY;
	float fNPRawZ;
	float fNPDeltaX;
	float fNPDeltaY;
	float fNPDeltaZ;
	float fNPSmoothX;
	float fNPSmoothY;
	float fNPSmoothZ;

} TRACKIRDATA, *LPTRACKIRDATA;

//
// Typedef for pointer to the notify callback function that is implemented within
// the client -- this function receives head tracker reports from the game client API
//
// MAY NEED TO ENABLE OTHER COMMENTED SETUPSTEPS LIKE STOP/START CURSOR
typedef NPRESULT (__stdcall *PF_NOTIFYCALLBACK)( unsigned short, unsigned short );

// Typedefs for game client API functions (useful for declaring pointers to these
// functions within the client for use during GetProcAddress() ops)
//
typedef NPRESULT (__stdcall *PF_NP_REGISTERWINDOWHANDLE)( HWND );
typedef NPRESULT (__stdcall *PF_NP_UNREGISTERWINDOWHANDLE)( void );
typedef NPRESULT (__stdcall *PF_NP_REGISTERPROGRAMPROFILEID)( unsigned short );
typedef NPRESULT (__stdcall *PF_NP_QUERYVERSION)( unsigned short* );
typedef NPRESULT (__stdcall *PF_NP_REQUESTDATA)( unsigned short );
typedef NPRESULT (__stdcall *PF_NP_GETSIGNATURE)( LPTRACKIRSIGNATURE );
typedef NPRESULT (__stdcall *PF_NP_GETDATA)( LPTRACKIRDATA );
typedef NPRESULT (__stdcall *PF_NP_REGISTERNOTIFY)( PF_NOTIFYCALLBACK );
typedef NPRESULT (__stdcall *PF_NP_UNREGISTERNOTIFY)( void );
typedef NPRESULT (__stdcall *PF_NP_STARTCURSOR)( void );
typedef NPRESULT (__stdcall *PF_NP_STOPCURSOR)( void );
typedef NPRESULT (__stdcall *PF_NP_RECENTER)( void );
typedef NPRESULT (__stdcall *PF_NP_STARTDATATRANSMISSION)( void );
typedef NPRESULT (__stdcall *PF_NP_STOPDATATRANSMISSION)( void );

#pragma pack( pop, npclient_h ) // Ensure previous pack value is restored


/////////////////
// Global Data ///////////////////////////////////////////////////////////////////
/////////////////

PF_NP_REGISTERWINDOWHANDLE       gpfNP_RegisterWindowHandle = NULL;
PF_NP_UNREGISTERWINDOWHANDLE     gpfNP_UnregisterWindowHandle = NULL;
PF_NP_REGISTERPROGRAMPROFILEID   gpfNP_RegisterProgramProfileID = NULL;
PF_NP_QUERYVERSION               gpfNP_QueryVersion = NULL;
PF_NP_REQUESTDATA                gpfNP_RequestData = NULL;
PF_NP_GETSIGNATURE               gpfNP_GetSignature = NULL;
PF_NP_GETDATA                    gpfNP_GetData = NULL;
PF_NP_STARTCURSOR                gpfNP_StartCursor = NULL;
PF_NP_STOPCURSOR                 gpfNP_StopCursor = NULL;
PF_NP_RECENTER	                 gpfNP_ReCenter = NULL;
PF_NP_STARTDATATRANSMISSION      gpfNP_StartDataTransmission = NULL;
PF_NP_STOPDATATRANSMISSION       gpfNP_StopDataTransmission = NULL;

HMODULE ghNPClientDLL = (HMODULE)NULL;

////////////////////////////////////////////////////
// NaturalPoint Game Client API function wrappers /////////////////////////////
////////////////////////////////////////////////////

NPRESULT __stdcall NP_RegisterWindowHandle(HWND hWnd)
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if (NULL != gpfNP_RegisterWindowHandle)
		result = (*gpfNP_RegisterWindowHandle)(hWnd);

	return result;
}


NPRESULT __stdcall NP_UnregisterWindowHandle()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if (NULL != gpfNP_UnregisterWindowHandle)
		result = (*gpfNP_UnregisterWindowHandle)();

	return result;
}


NPRESULT __stdcall NP_RegisterProgramProfileID(unsigned short wPPID)
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if (NULL != gpfNP_RegisterProgramProfileID)
		result = (*gpfNP_RegisterProgramProfileID)(wPPID);

	return result;
}


NPRESULT __stdcall NP_QueryVersion(unsigned short* pwVersion)
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if (NULL != gpfNP_QueryVersion)
		result = (*gpfNP_QueryVersion)(pwVersion);

	return result;
}


NPRESULT __stdcall NP_RequestData(unsigned short wDataReq)
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if (NULL != gpfNP_RequestData)
		result = (*gpfNP_RequestData)(wDataReq);

	return result;
} // NP_RequestData()

NPRESULT __stdcall NP_GetSignature(LPTRACKIRSIGNATURE pSignature)
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if (NULL != gpfNP_GetSignature)
		result = (*gpfNP_GetSignature)(pSignature);

	return result;
}


NPRESULT __stdcall NP_GetData(LPTRACKIRDATA pTID)
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if (NULL != gpfNP_GetData)
		result = (*gpfNP_GetData)(pTID);

	return result;
}


NPRESULT __stdcall NP_StartCursor()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if (NULL != gpfNP_StartCursor)
		result = (*gpfNP_StartCursor)();

	return result;
}


NPRESULT __stdcall NP_StopCursor()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if (NULL != gpfNP_StopCursor)
		result = (*gpfNP_StopCursor)();

	return result;
}


NPRESULT __stdcall NP_ReCenter()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if (NULL != gpfNP_ReCenter)
		result = (*gpfNP_ReCenter)();

	return result;
}


NPRESULT __stdcall NP_StartDataTransmission()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if (NULL != gpfNP_StartDataTransmission)
		result = (*gpfNP_StartDataTransmission)();

	return result;
}


NPRESULT __stdcall NP_StopDataTransmission()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if (NULL != gpfNP_StopDataTransmission)
		result = (*gpfNP_StopDataTransmission)();

	return result;
}


// NPClientInit() -- Loads the DLL and retrieves pointers to all exports
NPRESULT NPClient_Init(LPTSTR pszDLLPath)
{
	NPRESULT result = NP_OK;
	TCHAR szFullPath[MAX_PATH];

	wcscpy_s(szFullPath, MAX_PATH, pszDLLPath);

	if (lstrlen(szFullPath) > 0)
	{
		wcscat(szFullPath,L"\\");
	}
	else {
		return NP_ERR_DLL_NOT_FOUND;
	}

	#if defined(_WIN64) || defined(__amd64__)
		wcscat(szFullPath, "NPClient64.dll");
	#else	    
		wcscat(szFullPath, L"NPClient.dll");
	#endif

	ghNPClientDLL = LoadLibrary(szFullPath);

	if (NULL != ghNPClientDLL)
	{
		// verify the dll signature
		gpfNP_GetSignature = (PF_NP_GETSIGNATURE)::GetProcAddress(ghNPClientDLL, "NP_GetSignature");

		SIGNATUREDATA pSignature;
		SIGNATUREDATA verifySignature;
		// init the signatures
		strcpy(verifySignature.DllSignature, "precise head tracking\n put your head into the game\n now go look around\n\n Copyright EyeControl Technologies");
		strcpy(verifySignature.AppSignature, "hardware camera\n software processing data\n track user movement\n\n Copyright EyeControl Technologies");
		// query the dll and compare the results
		NPRESULT vresult = NP_GetSignature(&pSignature);
		if (vresult == NP_OK)
		{
			if ((strcmp(verifySignature.DllSignature, pSignature.DllSignature) == 0)
				&& (strcmp(verifySignature.AppSignature, pSignature.AppSignature) == 0))
			{
				result = NP_OK;

				// Get addresses of all exported functions
				gpfNP_RegisterWindowHandle = (PF_NP_REGISTERWINDOWHANDLE)::GetProcAddress(ghNPClientDLL, "NP_RegisterWindowHandle");
				gpfNP_UnregisterWindowHandle = (PF_NP_UNREGISTERWINDOWHANDLE)::GetProcAddress(ghNPClientDLL, "NP_UnregisterWindowHandle");
				gpfNP_RegisterProgramProfileID = (PF_NP_REGISTERPROGRAMPROFILEID)::GetProcAddress(ghNPClientDLL, "NP_RegisterProgramProfileID");
				gpfNP_QueryVersion = (PF_NP_QUERYVERSION)::GetProcAddress(ghNPClientDLL, "NP_QueryVersion");
				gpfNP_RequestData = (PF_NP_REQUESTDATA)::GetProcAddress(ghNPClientDLL, "NP_RequestData");
				gpfNP_GetData = (PF_NP_GETDATA)::GetProcAddress(ghNPClientDLL, "NP_GetData");
				gpfNP_StartCursor = (PF_NP_STARTCURSOR)::GetProcAddress(ghNPClientDLL, "NP_StartCursor");
				gpfNP_StopCursor = (PF_NP_STOPCURSOR)::GetProcAddress(ghNPClientDLL, "NP_StopCursor");
				gpfNP_ReCenter = (PF_NP_RECENTER)::GetProcAddress(ghNPClientDLL, "NP_ReCenter");
				gpfNP_StartDataTransmission = (PF_NP_STARTDATATRANSMISSION)::GetProcAddress(ghNPClientDLL, "NP_StartDataTransmission");
				gpfNP_StopDataTransmission = (PF_NP_STOPDATATRANSMISSION)::GetProcAddress(ghNPClientDLL, "NP_StopDataTransmission");
			}
			else
			{
				printf("NPTrackIR Program Not Running\n");
				result = NP_ERR_DLL_NOT_FOUND;
				return NP_ERR_DLL_NOT_FOUND;
			}
		}
		else
		{
			printf("NP Get signature failed\n");
			result = NP_ERR_DLL_NOT_FOUND;
		}
	}
	else {
		DWORD myerror = GetLastError();
		printf("DLL Load Failed\nError Code: %d\n", myerror);
		result = NP_ERR_DLL_NOT_FOUND;
	}

	return result;
}