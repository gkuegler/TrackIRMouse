#ifndef TRACKIRMOUSE_THREADS_H
#define TRACKIRMOUSE_THREADS_H

#include <Windows.h>
#include <wx/thread.h>

class cFrame;
class cPanel;

class TrackThread : public wxThread {
 public:
  cFrame* m_pHandler = nullptr;
  HWND m_hWnd;

  TrackThread(cFrame* m_pHandler, HWND hWnd);
  ~TrackThread();

  ExitCode Entry();
};

class WatchdogThread : public wxThread {
 public:
  cFrame* m_pHandler = nullptr;
  HANDLE m_hPipe = INVALID_HANDLE_VALUE;

  WatchdogThread(cFrame* pHandler);
  ~WatchdogThread();

  ExitCode Entry();
};

#endif /* TRACKIRMOUSE_THREADS_H */
