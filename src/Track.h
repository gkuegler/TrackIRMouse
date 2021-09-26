#pragma once
#ifndef TRACKIRMOUSE_TRACK_H
#define TRACKIRMOUSE_TRACK_H

#include <wx/wx.h>
#include <windows.h>

int trackInitialize(wxEvtHandler* m_parent, HWND hWnd);
int trackStart();

#endif /* TRACKIRMOUSE_TRACK_H */