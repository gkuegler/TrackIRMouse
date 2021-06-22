#ifndef TRACKIRMOUSE_WATCHDOG_H
#define TRACKIRMOUSE_WATCHDOG_H

namespace WatchDog
{
    HANDLE startWatchdog();
    DWORD WINAPI WDInstanceThread(LPVOID);
    int WDServe(HANDLE);
    void WDHandleMsg(const char*, char*, LPDWORD);
}
#endif /* TRACKIRMOUSE_WATCHDOG_H */