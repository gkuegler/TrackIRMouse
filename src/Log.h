#pragma once
#ifndef TRACKIRMOUSE_LOG_H
#define TRACKIRMOUSE_LOG_H

#include <wx/string.h>
#include <wx/log.h>
#include <wx/file.h>
#include <wx/wx.h>

#include <string>

//////////////////////////////////////////////////////////////////////
//        Functions Logging Specifically to the Text Control        //
//////////////////////////////////////////////////////////////////////

// Scott Meyers universal reference tips!
template <typename T>
void LogToWix(T&& msg)
{
    wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD);
    evt->SetString(std::forward<T>(msg));
    wxTheApp->QueueEvent(evt);
}

template <typename T>
void LogToWixError(T&& msg)
{
    wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD);
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
    void DoLogText(const wxString& msg);

};

#endif /* TRACKIRMOUSE_LOG_H */