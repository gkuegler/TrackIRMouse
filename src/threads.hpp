#ifndef TRACKIRMOUSE_THREADS_H
#define TRACKIRMOUSE_THREADS_H

#include <Windows.h>
#include <wx/thread.h>

#include <memory>

#include "config.hpp"
#include "handlers.hpp"
#include "trackers.hpp"

class cFrame;
class cPanel;

class TrackThread : public wxThread {
 public:
  cFrame* m_pHandler = nullptr;
  std::shared_ptr<trackers::TrackIR> m_tracker;
  std::shared_ptr<handlers::MouseHandler> m_handler;

 private:
  HWND m_hWnd;
  std::shared_ptr<config::Config> m_config;

 public:
  TrackThread(cFrame* m_pHandler, HWND hWnd,
              std::shared_ptr<config::Config> config);
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
