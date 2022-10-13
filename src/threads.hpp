#ifndef TRACKIRMOUSE_THREADS_H
#define TRACKIRMOUSE_THREADS_H

#include <Windows.h>
#include <wx/thread.h>

#include <memory>

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

class ControlServerThread : public wxThread {
 public:
  cFrame* m_pHandler = nullptr;

  ControlServerThread(cFrame* pHandler);
  ~ControlServerThread();
  ExitCode Entry();

  // private:
  // std::unique_ptr<Watchdog> watchdog = nullptr;
};

#endif /* TRACKIRMOUSE_THREADS_H */
