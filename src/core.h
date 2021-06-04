
// Header defined interfaces for core app functionality
// each of these functions modifies or accesses the global
// array of dislpay information.

#ifndef TRACKIRMOUSE_CORE_H
#define TRACKIRMOUSE_CORE_H

#include "Config.h"

int WinSetup();
void DisplaySetup(int, CConfig&);
void MouseMove(int, float, float);

#endif /* TRACKIRMOUSE_CORE_H */