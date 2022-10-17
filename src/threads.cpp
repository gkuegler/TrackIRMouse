#include "threads.hpp"

#include <wx/thread.h>

#include <memory>

#include "gui.hpp"
#include "pipeserver.hpp"
#include "types.hpp"

//////////////////////////////////////////////////////////////////////
//                           TrackThread                            //
//////////////////////////////////////////////////////////////////////

TrackThread::TrackThread(Frame* p_window_handler,
                         HWND hWnd,
                         std::shared_ptr<config::Config> config)
  : wxThread()
{
  p_window_handler_ = p_window_handler;
  hWnd_ = hWnd;
  config_ = config; // create copy of user data & etc. for tracking threaded
}

TrackThread::~TrackThread()
{
  // Threads are detached anddelete themselves when they are done running.
  // Will need to provide locks in the future with critical sections
  // https://docs.wxwidgets.org/3.0/classwx_thread.html
  wxCriticalSectionLocker enter(p_window_handler_->p_thread_cs);
  p_window_handler_->p_track_thread_ = NULL;
}

// TODO: move to polymorphic track object created by dependency injection
wxThread::ExitCode
TrackThread::Entry()
{
  auto profile = config_->GetActiveProfile();
  auto path = config_->env_data.track_ir_dll_path;
  auto quit_on_no_trackir = config_->user_data.quit_on_loss_of_trackir;

  handler_ = std::make_shared<handlers::MouseHandler>();
  tracker_ = std::make_shared<trackers::TrackIR>(
    hWnd_, path, profile.profile_id, handler_.get());

  if (false == config::ValidateUserInput(profile.displays)) {
    throw std::runtime_error("invalid user profile data");
  }

  try {
    if (tracker_->start() == retcode::fail &&
        config_->user_data.quit_on_loss_of_trackir) {
      spdlog::trace("quitting on loss of track ir");
      SendThreadMessage(msgcode::close_app, "");
    }
  } catch (const std::runtime_error& e) {
    spdlog::error("tracking exception: {}", e.what());
  }

  spdlog::trace("track thread is closing");
  return NULL;
}
//////////////////////////////////////////////////////////////////////
//                         WatchdogThread                           //
//////////////////////////////////////////////////////////////////////

ControlServerThread::ControlServerThread(Frame* p_window_handler)
  : wxThread()
{
  p_window_handler_ = p_window_handler;
}

ControlServerThread::~ControlServerThread()
{
  // Threads are detached anddelete themselves when they are done running.
  // TODO: Will need to provide different locks in the future with critical
  // sections https://docs.wxwidgets.org/3.0/classwx_thread.html
  wxCriticalSectionLocker enter(p_window_handler_->p_thread_cs);
  p_window_handler_->p_server_thread_ = NULL;
  // end of critical section
}

wxThread::ExitCode
ControlServerThread::Entry()
{
  try {
    // TODO: make name part of construction
    auto server = PipeServer();
    server.Serve("watchdog");
  } catch (const std::runtime_error& e) {
    spdlog::warn(e.what());
  }
  return NULL;
}
