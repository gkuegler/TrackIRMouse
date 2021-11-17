#ifndef TRACKIRMOUSE_TRACK_H
#define TRACKIRMOUSE_TRACK_H

#include "Config.h"
#include "Display.h"

#include <wx/wx.h>
#include <windows.h>

void TR_Initialize(HWND hWnd, CConfig config);
void TR_TrackStart(CConfig config);
void TR_TrackStop(CConfig config);

// class CTracker
// {
// public:
//     bool m_IsInitialized = false;
//     bool m_IsTracking = false;
//     HANDLE m_hWatchdogThread = NULL;

//     // Passing configuration by value is an intentional choice here
//     CTracker(wxEvtHandler* m_parent, HWND hWnd, const CConfig config);
//     int trackStart(const CConfig config);
//     void trackStop();

// private:
//     std::vector<CDisplay> m_displays;

//     signed int m_virtualOriginX = 0;
//     signed int m_virtualOriginY = 0;
//     float m_xPixelAbsoluteSlope = 0;
//     float m_yPixelAbsoluteSlope = 0;

//     void WinSetup(CConfig);
//     void DisplaySetup(const CConfig);
//     void MouseMove(int, float, float);

//     BOOL PopulateVirtMonitorBounds(HMONITOR, HDC, LPRECT);
//     static BOOL CALLBACK WrapperPopulateVirtMonitorBounds(HMONITOR, HDC, LPRECT, LPARAM);
// };

#endif /* TRACKIRMOUSE_TRACK_H */