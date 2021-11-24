#ifndef TRACKIRMOUSE_LOG_H
#define TRACKIRMOUSE_LOG_H

#define FMT_HEADER_ONLY
// #include <fmt\xchar.h>
#include <fmt\format.h>

#define SPDLOG_NO_THREAD_ID
#define SPDLOG_NO_ATOMIC_LEVELS
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <wx/file.h>
#include <wx/log.h>
#include <wx/string.h>
#include <wx/wx.h>

#include <string>

//////////////////////////////////////////////////////////////////////
//        Functions Logging Specifically to the Text Control        //
//////////////////////////////////////////////////////////////////////

// Scott Meyers universal reference tips!
template <typename T> void LogToWix(T &&msg)
{
    // std::string msg2(msg);
    // logger->info(msg2.c_str());
    // spdlog::get("logger1")->info("hnlogger->info<T>(msg);
    spdlog::get("mainlogger")->info<T>(msg);
    wxThreadEvent *evt = new wxThreadEvent(wxEVT_THREAD);
    evt->SetString(std::forward<T>(msg));
    wxTheApp->QueueEvent(evt);
}

template <typename T> void LogToWixError(T &&msg)
{
    // logger->warn(msg);
    wxThreadEvent *evt = new wxThreadEvent(wxEVT_THREAD);
    evt->SetString(std::forward<T>(msg));
    evt->SetExtraLong(1);
    wxTheApp->QueueEvent(evt);
}

//////////////////////////////////////////////////////////////////////
//     Functions for General-purpose Logging, to File, etc ...      //
//////////////////////////////////////////////////////////////////////

class CMyLogger : public wxLog
{
  public:
    wxString m_filename = "log-trackir.txt";
    wxFile m_file;

    CMyLogger();
    void DoLogText(const wxString &msg);
};

#endif /* TRACKIRMOUSE_LOG_H */
