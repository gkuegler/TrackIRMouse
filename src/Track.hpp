#ifndef TRACKIRMOUSE_TRACK_H
#define TRACKIRMOUSE_TRACK_H

#include <windows.h>

#include "Config.hpp"

int TR_Initialize(HWND hWnd, CConfig config);
int TR_TrackStart(CConfig config);
void TR_TrackStop();

#endif /* TRACKIRMOUSE_TRACK_H */
