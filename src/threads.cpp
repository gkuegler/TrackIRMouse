#include "threads.hpp"

#include <wx/thread.h>

#include <memory>

#include "messages.hpp"
#include "pipeserver.hpp"
#include "types.hpp"
#include "ui-frame.hpp"

//////////////////////////////////////////////////////////////////////
//                           TrackThread                            //
//////////////////////////////////////////////////////////////////////

TrackThread::TrackThread(Frame* p_window_handler,
                         HWND hWnd,
                         settings::Settings settings)
  : wxThread()
{
  p_window_handler_ = p_window_handler;
  hWnd_ = hWnd;
  // TODO: only copy the settings and active
  // profile for speed and memory optimization.
  settings_ = settings; // create copy of user data for tracking thread

  // TODO: use a unique pointer instead
  handler_ =
    std::make_shared<handlers::MouseHandler>(settings.GetActiveProfile());
  tracker_ = std::make_shared<trackers::TrackIR>(handler_.get());
}

TrackThread::~TrackThread()
{
  // Threads run detached and delete themselves when they complete their entry
  // method. Make sure thread object does not
  // https://docs.wxwidgets.org/3.0/classwx_thread.html
  wxCriticalSectionLocker enter(p_window_handler_->p_cs_track_thread);
  p_window_handler_->track_thread_ = NULL;
}

wxThread::ExitCode
TrackThread::Entry()
{
  auto profile = settings_.GetActiveProfile();

  // TODO: check validation is correct
  if (false == profile.ValidateParameters()) {
    spdlog::error("invalid user profile data");
    return NULL;
  }

  // initialize resources
  try {
    // handler_ = std::make_shared<handlers::MouseHandler>();
    tracker_->initialize(hWnd_,
                         settings_.auto_find_track_ir_dll,
                         settings_.track_ir_dll_folder,
                         profile.profile_id);

    // run the main tracking loop
    auto result = tracker_->start();

    if (retcode::track_ir_loss == result && settings_.quit_on_loss_of_trackir) {
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

// TODO: make a restarting service for checkbox ini settings
ControlServerThread::ControlServerThread(Frame* p_window_handler,
                                         std::string name)
  : wxThread()
{

  p_window_handler_ = p_window_handler;
  server_name_ = name;
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
    auto server = PipeServer();
    server.Serve(server_name_);
  } catch (const std::runtime_error& ex) {
    spdlog::error(ex.what());
  }
  return NULL;
}
