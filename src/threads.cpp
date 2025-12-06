#include "threads.hpp"

#include <wx/thread.h>

#include <memory>

#include "messages.hpp"
#include "pipeserver.hpp"
#include "types.hpp"
#include "ui/frame.hpp"

//////////////////////////////////////////////////////////////////////
//                           TrackThread                            //
//////////////////////////////////////////////////////////////////////

ThreadHeadTracking::ThreadHeadTracking(ThreadHeadTracking** self,
                                       wxCriticalSection* lock,
                                       HWND hWnd,
                                       Settings s)
  : self(self)
  , lock(lock)
  , hWnd(hWnd)
  , settings(s)
  , wxThread()
{
  // TODO: use a unique pointer instead?
  handler_ = std::make_shared<handlers::MouseHandler>(settings.GetActiveProfileRef());
  tracker = std::make_shared<trackers::TrackIR>(handler_.get());
}

ThreadHeadTracking::~ThreadHeadTracking()
{
  // Threads run detached and delete themselves when they complete their entry
  // method. Make sure thread object does not
  // https://docs.wxwidgets.org/3.0/classwx_thread.html
  wxCriticalSectionLocker enter(*lock);
  *self = NULL;
}

wxThread::ExitCode
ThreadHeadTracking::Entry()
{
  auto profile = settings.GetActiveProfileRef();
  const unsigned long retry_time = 1500; // ms

  // TODO: check validation is correct
  if (false == profile.ValidateParameters()) {
    spdlog::error("invalid user profile data");
    return NULL;
  }

  while (false == wxThread::TestDestroy()) {
    try {
      // handler_ = std::make_shared<handlers::MouseHandler>();return
      tracker->initialize(
        hWnd, settings.auto_find_track_ir_dll, settings.track_ir_dll_folder, profile.title_id);

      // run the main tracking loop
      tracker->start();

      // I don't think I need to quit on the loss track IR now that I've
      // implemented a retry strategy.
      // if (retcode::track_ir_loss == result &&
      //     settings_.quit_on_loss_of_trackir) {
      //   spdlog::trace("quitting on loss of track ir");
      //   SendThreadMessage(msgcode::close_app, "");
      // }
    } catch (const trackers::error_device_not_present& ex) {
      if (settings.auto_retry) {
        spdlog::warn("Device not present. Retrying...");
        Sleep(retry_time);
        continue;
      } else {
        spdlog::error(ex.what());
        return NULL;
      }
    } catch (const std::runtime_error& ex) {
      // TODO: when dll path is changed by user this doesn't log?
      spdlog::error("Couldn't start tracker:\n{}", ex.what());
    }

    spdlog::trace("track thread is closing");
    return NULL;
  }
}
wxThreadError
ThreadHeadTracking::Delete(ExitCode* rc, wxThreadWait waitMode)
{
  // Custom thread stopping hook to keed thread interface consistent.
  // Tracker uses an atomic<bool> instead of calling wxTestDestroy for
  // performace reasons.
  tracker->stop();

  // Ensure base class works as intended.
  // return wxThread::Delete(rc, waitMode);
  return wxTHREAD_NO_ERROR;
}
//////////////////////////////////////////////////////////////////////
//                         WatchdogThread                           //
//////////////////////////////////////////////////////////////////////

// TODO: make a restarting service for checkbox ini settings
ThreadPipeServer::ThreadPipeServer(MainWindow* p_window_handler, std::string name)
  : wxThread()
{

  p_window_handler_ = p_window_handler;
  server_name_ = name;
  spdlog::trace("pipe server constructed");
}

ThreadPipeServer::~ThreadPipeServer()
{
  spdlog::trace("pipe server destructed");
  // Threads are detached and delete themselves when they are done running.
  wxCriticalSectionLocker enter(p_window_handler_->cs_pipe_thread);
  p_window_handler_->pipe_server_thread = NULL;
}

wxThread::ExitCode
ThreadPipeServer::Entry()
{
  try {
    auto server = PipeServer(server_name_);
    while (false == wxThread::TestDestroy()) {
      server.ServeOneClient();
    }
  } catch (const std::runtime_error& ex) {
    spdlog::error(ex.what());
  }
  return NULL;
}
