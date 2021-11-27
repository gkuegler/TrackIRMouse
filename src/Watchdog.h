#ifndef TRACKIRMOUSE_WATCHDOG_H
#define TRACKIRMOUSE_WATCHDOG_H

#include <Windows.h>

namespace WatchDog
{
HANDLE StartWatchdog();
DWORD WINAPI InstanceThread(LPVOID);
void Serve(HANDLE);
void HandleMsg(const char *, char *, LPDWORD);
} // namespace WatchDog

#endif /* TRACKIRMOUSE_WATCHDOG_H */
