#include "threads.hpp"

#include <wx/thread.h>

#include <memory>

#include "pipeserver.hpp"
#include "types.hpp"
#include "ui-frame.hpp"

// Error Message Boxes

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
  config_ = config; // create copy of user data for tracking thread

  // TODO: use a unique pointer instead
  handler_ = std::make_shared<handlers::MouseHandler>();
  tracker_ = std::make_shared<trackers::TrackIR>(handler_.get());
}

TrackThread::~TrackThread()
{
  // Threads run detached and delete themselves when they complete their entry
  // method. Make sure thread object does not
  // https://docs.wxwidgets.org/3.0/classwx_thread.html
  wxCriticalSectionLocker enter(p_window_handler_->p_cs_track_thread);
  p_window_handler_->p_track_thread_ = NULL;
}

wxThread::ExitCode
TrackThread::Entry()
{
  auto profile = config_->GetActiveProfile();
  auto dll_folder = config_->user_data.track_ir_dll_folder;
  auto auto_find_dll = config_->user_data.auto_find_track_ir_dll;
  auto quit_on_no_trackir = config_->user_data.quit_on_loss_of_trackir;

  // TODO: check validation is correct
  if (false == config::ValidateUserInput(profile.displays)) {
    spdlog::error("invalid user profile data");
    return NULL;
  }

  // initialize resources
  try {
    // handler_ = std::make_shared<handlers::MouseHandler>();
    tracker_->initialize(hWnd_, auto_find_dll, dll_folder, profile.profile_id);

    // run the main tracking loop
    auto result = tracker_->start();

    if (retcode::track_ir_loss == result &&
        config_->user_data.quit_on_loss_of_trackir) {
      spdlog::trace("quitting on loss of track ir");
      SendThreadMessage(msgcode::close_app, "");
    }
  } catch (const std::runtime_error& e) {
    // TODO: when dll path is changed by user this doesn't log
    spdlog::error("Error starting tracker: {}", e.what());
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
  wxCriticalSectionLocker enter(p_window_handler_->p_cs_track_thread);
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
    spdlog::error(e.what());
  }
  return NULL;
}
