#pragma once
#ifndef TRACKIRMOUSE_LOG_H
#define TRACKIRMOUSE_LOG_H

#include <string>

#include <wx/string.h>
#include <wx/log.h>
#include <wx/file.h>


 void logToWix(std::string msg);
 void logToWix(std::wstring msg);

 void logToWix(const char* msg);
 void logToWix(const wchar_t* msg);

 class CMyLogger : public wxLog
 {
 public:
	 wxString m_filename = "log-trackir.txt";
	 wxFile m_file;

	 CMyLogger();
	 void DoLogText(const wxString& msg);

 };

#endif /* TRACKIRMOUSE_LOG_H */