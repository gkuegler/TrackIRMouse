#ifndef TRACKIRMOUSE_THREADS_H
#define TRACKIRMOUSE_THREADS_H

#include "windows-wrapper.hpp"
#include <wx/thread.h>

#include <memory>

#include "handlers.hpp"
#include "settings.hpp"
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
  settings::Settings settings_;

public:
  TrackThread(Frame* window_handler, HWND hWnd, settings::Settings settings);
  ~TrackThread();
  ExitCode Entry();
};

class ControlServerThread : public wxThread
{
public:
  Frame* p_window_handler_ = nullptr;
  std::string server_name_ = "defaultname";

  ControlServerThread(Frame* window_handler, std::string name);
  ~ControlServerThread();
  ExitCode Entry();
};

#endif /* TRACKIRMOUSE_THREADS_H */
