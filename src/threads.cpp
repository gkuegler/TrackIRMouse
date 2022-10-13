#include "threads.hpp"

#include <wx/thread.h>

#include "gui.hpp"
#include "pipeserver.hpp"
#include "types.hpp"

//////////////////////////////////////////////////////////////////////
//                           TrackThread                            //
//////////////////////////////////////////////////////////////////////

TrackThread::TrackThread(cFrame* pHandler, HWND hWnd) : wxThread() {
  m_pHandler = pHandler;
  m_hWnd = hWnd;
}

TrackThread::~TrackThread() {
  // Threads are detached anddelete themselves when they are done running.
  // Will need to provide locks in the future with critical sections
  // https://docs.wxwidgets.org/3.0/classwx_thread.html
  wxCriticalSectionLocker enter(m_pHandler->m_pThreadCS);
  m_pHandler->m_pTrackThread = NULL;
}

void CloseApplication() {
  wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD);
  evt->SetInt(1);
  wxTheApp->QueueEvent(evt);
}

// TODO: move to polymorphic track object created by dependency injection
wxThread::ExitCode TrackThread::Entry() {
  auto config = config::Get();
  auto profile = config->GetActiveProfile();
  auto path = config->envData.trackIrDllPath;
  auto quit_on_no_trackir = config->userData.quitOnLossOfTrackIr;
  if (track::Initialize(m_hWnd, profile, path) == retcode::fail) {
    return NULL;
  }

  // This is the loop function
  // TODO: use exceptions
  if (track::Start() == retcode::fail && quit_on_no_trackir) {
    spdlog::trace("quitting on loss of track ir");
    CloseApplication();
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
    PipeServer server;
    server.Serve("watchdog");
  } catch (const std::runtime_error& e) {
    spdlog::warn(e.what());
  }
  return NULL;
}
