#include "threads.hpp"

#include <wx/thread.h>

#include "gui.hpp"
#include "watchdog.hpp"
#include "types.hpp"

//////////////////////////////////////////////////////////////////////
//                           TrackThread                            //
//////////////////////////////////////////////////////////////////////

TrackThread::TrackThread(cFrame* pHandler, HWND hWnd) : wxThread() {
  m_pHandler = pHandler;
  m_hWnd = hWnd;
}

TrackThread::~TrackThread() {
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

wxThread::ExitCode TrackThread::Entry() {
  auto profile = config::GetActiveProfile();
  auto path = config::GetEnvironmentData().trackIrDllPath;
  auto usr = config::GetUserData();
  if (track::Initialize(m_hWnd, profile, path) == retcode::fail) {
    return NULL;
  }

  // This is the loop function
  if (track::TrackStart() == retcode::fail && usr.quitOnLossOfTrackIr) {
    spdlog::trace("quitting on loss of track ir");
    CloseApplication();
  }
  spdlog::trace("track thread is closing");

  return NULL;
}
//////////////////////////////////////////////////////////////////////
//                         WatchdogThread                           //
//////////////////////////////////////////////////////////////////////

WatchdogThread::WatchdogThread(cFrame* pHandler) : wxThread() {
  m_pHandler = pHandler;
  m_hPipe = WatchDog::InitializeWatchdog();
}

WatchdogThread::~WatchdogThread() {
  // Will need to provide locks in the future with critical sections
  // https://docs.wxwidgets.org/3.0/classwx_thread.html
  wxCriticalSectionLocker enter(m_pHandler->m_pThreadCS);
  m_pHandler->m_pWatchdogThread = NULL;

  // close named pipe
  if (0 == CloseHandle(m_hPipe)) {
    spdlog::error(
        "Could not close named pipe on destruction of watchdog. GLE={}",
        GetLastError());
  }
}

wxThread::ExitCode WatchdogThread::Entry() {
  if (m_hPipe) {
    WatchDog::Serve(m_hPipe);
  } else {
    spdlog::warn("watchdog thread already stopped");
  }
  return NULL;
}
