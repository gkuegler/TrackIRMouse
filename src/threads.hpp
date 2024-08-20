#pragma once
#ifndef TIRMOUSE_THREADS_H
#define TIRMOUSE_THREADS_H

#include <wx/thread.h>

#include <memory>

#include "handlers.hpp"
#include "settings.hpp"
#include "trackers.hpp"
#include "windows-wrapper.hpp"


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
  // The thread is responsible for setting '*thread' to 'nullptr' in its
  // destructor.

  // A flag is used because we have to enter the mutex to
  // stop the thread and then leave the mutex to allow it to stop.
  bool thread_was_alive = false;

  { // enter critical section
    wxCriticalSectionLocker enter(cs);
    if (thread) {
      thread->Delete();
      thread_was_alive = true;
    }
  } // leave critical section

  // Allow thread a timeout to gracefully exit.
  if (thread_was_alive) {

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
      spdlog::error(
        "Thread not stoped gracefully within timeout. Thread may still be "
        "running and not responsive. Attempting to kill thread.");
    }
    wxCriticalSectionLocker enter(cs);
    if (thread) {
      thread->Kill();
    }
  }
};

#endif /* TIRMOUSE_THREADS_H */
