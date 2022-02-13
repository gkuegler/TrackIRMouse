#include "threads.hpp"

#include <wx/thread.h>

#include "gui.hpp"
#include "watchdog.hpp"

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
  CConfig config = GetGlobalConfigCopy();
  if (0 != TR_Initialize(m_hWnd, config)) {
    return NULL;
  }

  // This is the loop function
  int result = TR_TrackStart(config);
  if (1 == result && config.data.quitOnLossOfTrackIr) {
    CloseApplication();
  }

  return NULL;
}
//////////////////////////////////////////////////////////////////////
//                         WatchdogThread                           //
//////////////////////////////////////////////////////////////////////

WatchdogThread::WatchdogThread(cFrame* pHandler) : wxThread() {
  m_pHandler = pHandler;
  m_hPipe = WatchDog::StartWatchdog();
}

WatchdogThread::~WatchdogThread() {
  // Will need to provide locks in the future with critical sections
  // https://docs.wxwidgets.org/3.0/classwx_thread.html
  wxCriticalSectionLocker enter(m_pHandler->m_pThreadCS);
  m_pHandler->m_pWatchdogThread = NULL;

  // TODO: close named pipe
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
