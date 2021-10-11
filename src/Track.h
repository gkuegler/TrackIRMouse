#pragma once
#ifndef TRACKIRMOUSE_TRACK_H
#define TRACKIRMOUSE_TRACK_H

#include <wx/wx.h>
#include <windows.h>
#include "Config.h"

class CTracker
{
public:
    bool m_IsInitialized = false;
    bool m_IsTracking = false;
    HANDLE m_hWatchdogThread = NULL;

    CTracker(wxEvtHandler* m_parent, HWND hWnd, CConfig* config);
    int trackStart(CConfig* config);
    void trackStop();
};

#endif /* TRACKIRMOUSE_TRACK_H */