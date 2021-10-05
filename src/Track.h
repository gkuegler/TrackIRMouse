#pragma once
#ifndef TRACKIRMOUSE_TRACK_H
#define TRACKIRMOUSE_TRACK_H

#include <wx/wx.h>
#include <windows.h>
#include "Config.h"

namespace Track
{
	int trackInitialize(wxEvtHandler* m_parent, HWND hWnd, CConfig* config);
	int trackStart(CConfig* config);
}

//class CTracker
//{
//public:
//    bool m_IsInitialized = false;
//    bool m_IsTracking = false;
//
//    int trackInitialize(wxEvtHandler* m_parent, HWND hWnd);
//    int trackStart();
//};

#endif /* TRACKIRMOUSE_TRACK_H */