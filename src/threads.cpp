#include "threads.hpp"

#include <wx/thread.h>

#include <memory>

#include "gui.hpp"
#include "pipeserver.hpp"
#include "types.hpp"

//////////////////////////////////////////////////////////////////////
//                           TrackThread                            //
//////////////////////////////////////////////////////////////////////

TrackThread::TrackThread(cFrame* pHandler, HWND hWnd,
                         std::shared_ptr<config::Config> config)
    : wxThread() {
  m_pHandler = pHandler;
  m_hWnd = hWnd;
  m_config = config;  // create copy of user data & etc. for tracking threaded
}

TrackThread::~TrackThread() {
  // Threads are detached anddelete themselves when they are done running.
  // Will need to provide locks in the future with critical sections
  // https://docs.wxwidgets.org/3.0/classwx_thread.html
  wxCriticalSectionLocker enter(m_pHandler->m_pThreadCS);
  m_pHandler->m_pTrackThread = NULL;
}

// TODO: move to polymorphic track object created by dependency injection
wxThread::ExitCode TrackThread::Entry() {
  auto profile = m_config->GetActiveProfile();
  auto path = m_config->envData.trackIrDllPath;
  auto quit_on_no_trackir = m_config->userData.quitOnLossOfTrackIr;

  m_handler = std::make_shared<handlers::MouseHandler>();
  m_tracker = std::make_shared<trackers::TrackIR>(
      m_hWnd, path, profile.profileId, m_handler.get());

  if (false == config::ValidateUserInput(profile.displays)) {
    throw std::runtime_error("invalid user profile data");
  }

  try {
    if (m_tracker->start() == retcode::fail &&
        m_config->userData.quitOnLossOfTrackIr) {
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

ControlServerThread::ControlServerThread(cFrame* pHandler) : wxThread() {
  m_pHandler = pHandler;
}

ControlServerThread::~ControlServerThread() {
  // Threads are detached anddelete themselves when they are done running.
  // TODO: Will need to provide different locks in the future with critical
  // sections https://docs.wxwidgets.org/3.0/classwx_thread.html
  wxCriticalSectionLocker enter(m_pHandler->m_pThreadCS);
  m_pHandler->m_pServerThread = NULL;
  // end of critical section
}

wxThread::ExitCode ControlServerThread::Entry() {
  try {
    // TODO: make name part of construction
    auto server = PipeServer();
    server.Serve("watchdog");
  } catch (const std::runtime_error& e) {
    spdlog::warn(e.what());
  }
  return NULL;
}
