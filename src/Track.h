#pragma once
#ifndef TRACKIRMOUSE_TRACK_H
#define TRACKIRMOUSE_TRACK_H

#include "Config.h"
#include "Display.h"

#include <wx/wx.h>
#include <windows.h>

class CTracker
{
public:
    bool m_IsInitialized = false;
    bool m_IsTracking = false;
    HANDLE m_hWatchdogThread = NULL;

    CTracker(wxEvtHandler* m_parent, HWND hWnd, CConfig* config);
    int trackStart(CConfig* config);
    void trackStop();

private:
    CDisplay m_displays[DEFAULT_MAX_DISPLAYS];

    signed int m_virtualOriginX = 0; 
    signed int m_virtualOriginY = 0;
    float m_xPixelAbsoluteSlope = 0;
    float m_yPixelAbsoluteSlope = 0;

    static BOOL CALLBACK WrapperPopulateVirtMonitorBounds(HMONITOR, HDC, LPRECT, LPARAM);
    BOOL PopulateVirtMonitorBounds(HMONITOR, HDC, LPRECT);
    void WinSetup();
    void DisplaySetup(CConfig*);
    void MouseMove(int, float, float);
};

#endif /* TRACKIRMOUSE_TRACK_H */