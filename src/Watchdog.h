#ifndef TRACKIRMOUSE_WATCHDOG_H
#define TRACKIRMOUSE_WATCHDOG_H

#include <Windows.h>

namespace WatchDog
{
    HANDLE StartWatchdog();
    DWORD WINAPI InstanceThread(LPVOID);
    int Serve(HANDLE);
    void HandleMsg(const char*, char*, LPDWORD);
}
#endif /* TRACKIRMOUSE_WATCHDOG_H */