#ifndef TRACKIRMOUSE_WATCHDOG_H
#define TRACKIRMOUSE_WATCHDOG_H

#include <Windows.h>

namespace WatchDog
{
    HANDLE WD_StartWatchdog();
    DWORD WINAPI WD_InstanceThread(LPVOID);
    int WD_Serve(HANDLE);
    void WD_HandleMsg(const char*, char*, LPDWORD);
}
#endif /* TRACKIRMOUSE_WATCHDOG_H */