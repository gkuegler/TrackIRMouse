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
  wxThreadError Delete(ExitCode* rc = NULL,
                       wxThreadWait waitMode = wxTHREAD_WAIT_DEFAULT);
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

// void
// DestroyThreadAndWait(wxThread* thread, wxCriticalSection mutex)
// {
//   bool wait_for_stop = false;
//   { // enter critical section
//     wxCriticalSectionLocker enter(mutex);
//     if (thread) {
//       thread->Delete();
//       wait_for_stop = true;
//     }
//   } // leave critical section

//   if (wait_for_stop) {
//     // TODO: set a timeout here? and close app if thread doesn't die
//     while (true) {
//       Sleep(8);
//       wxCriticalSectionLocker enter(mutex);
//       if (thread == NULL) {
//         break;
//       }
//     }
//   }
// };

// template<typename ThreadType>
// void
// StartThread(wxThread* thread, wxCriticalSection mutex)
// {
//   try {
//     auto settings = settings::GetCopy();
//     settings.ApplyNecessaryDefaults();
//     // TODO: How to forward these arguments?
//     thread = new ThreadType(this, this->GetHandle(), settings);
//   } catch (const std::runtime_error& e) {
//     spdlog::error(e.what());
//     return;
//   }

//   if (track_thread_->Run() == wxTHREAD_NO_ERROR) { // returns immediately
//     spdlog::info("Started Mouse.");
//   } else {
//     spdlog::error("Can't run the tracking thread!");
//     delete track_thread_;
//     // If I leave this commented memory leaks will manifest as bugs during
//     the
//     // track restart where I can catch them?
//     // track_thread_ = nullptr; // thread destructor should do this anyway?
//     return;
//   }
// };

#endif /* TRACKIRMOUSE_THREADS_H */
