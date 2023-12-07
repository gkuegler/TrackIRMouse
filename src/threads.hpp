#ifndef TRACKIRMOUSE_THREADS_H
#define TRACKIRMOUSE_THREADS_H

#include "windows-wrapper.hpp"
#include <wx/thread.h>

#include <memory>

#include "handlers.hpp"
#include "settings.hpp"
#include "trackers.hpp"

class MainWindow;

class ThreadHeadTracking : public wxThread
{
public:
  MainWindow* p_window_handler_ = nullptr;
  std::shared_ptr<trackers::TrackIR> tracker_;
  std::shared_ptr<handlers::MouseHandler> handler_;

private:
  HWND hWnd_;
  settings::Settings settings_;

public:
  ThreadHeadTracking(MainWindow* window_handler,
                     HWND hWnd,
                     settings::Settings settings);
  ~ThreadHeadTracking();
  ExitCode Entry();
  wxThreadError Delete(ExitCode* rc = NULL,
                       wxThreadWait waitMode = wxTHREAD_WAIT_DEFAULT);
};

class ThreadPipeServer : public wxThread
{
public:
  MainWindow* p_window_handler_ = nullptr;
  std::string server_name_ = "LoremIpsum";

  ThreadPipeServer(MainWindow* window_handler, std::string name);
  ~ThreadPipeServer();
  ExitCode Entry();
};

template<typename T>
void
GracefullyDeleteThreadAndWait(T*& thread, wxCriticalSection& cs)
{
  // Threads run in detached mode by default.
  // The thread is responsible for setting '*thread' to NULL in its
  // destructor.
  bool wait_for_stop = false;
  { // enter critical section
    wxCriticalSectionLocker enter(cs);
    if (thread) {
      thread->Delete();
      wait_for_stop = true;
    }
  } // leave critical section

  if (wait_for_stop) {

    // A crude one second timeout.
    constexpr const int time_cs = 100; // in centi-seconds
    int i = 0;
    for (; i < 100; i++) {
      Sleep(8);
      wxCriticalSectionLocker enter(cs);
      if (thread == NULL) {
        break;
      }
    }
    if (i == time_cs) {
      spdlog::error("Thread not stoped within timeout. Thread may still be "
                    "running and not responsive.");
    }
    wxCriticalSectionLocker enter(cs);
    if (thread) {
      thread->Kill();
    }
  }
};

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
