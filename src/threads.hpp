#ifndef TRACKIRMOUSE_THREADS_H
#define TRACKIRMOUSE_THREADS_H


#include "windows-wrapper.hpp"
#include <wx/thread.h>

#include <memory>

#include "config.hpp"
#include "handlers.hpp"
#include "trackers.hpp"

class Frame;
class Panel;

class TrackThread : public wxThread
{
public:
  Frame* p_window_handler_ = nullptr;
  std::shared_ptr<trackers::TrackIR> tracker_;
  std::shared_ptr<handlers::MouseHandler> handler_;

private:
  HWND hWnd_;
  std::shared_ptr<config::Config> config_;

public:
  TrackThread(Frame* window_handler,
              HWND hWnd,
              std::shared_ptr<config::Config> config);
  ~TrackThread();
  ExitCode Entry();
};

class ControlServerThread : public wxThread
{
public:
  Frame* p_window_handler_ = nullptr;

  ControlServerThread(Frame* window_handler);
  ~ControlServerThread();
  ExitCode Entry();

  // private:
  // std::unique_ptr<Watchdog> watchdog = nullptr;
};

#endif /* TRACKIRMOUSE_THREADS_H */
