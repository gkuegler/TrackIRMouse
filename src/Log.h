#pragma once
#ifndef TRACKIRMOUSE_LOG_H
#define TRACKIRMOUSE_LOG_H

#include <wx/string.h>
#include <wx/log.h>
#include <wx/file.h>

#include <string>

 void LogToWix(std::string msg);
 void LogToWix(std::wstring msg);

 void LogToWix(const char* msg);
 void LogToWix(const wchar_t* msg);

 void LogToWixError(std::string msg);
 void LogToWixError(std::wstring msg);

 void LogToWixError(const char* msg);
 void LogToWixError(const wchar_t* msg);

 class CMyLogger : public wxLog
 {
 public:
	 wxString m_filename = "log-trackir.txt";
	 wxFile m_file;

	 CMyLogger();
	 void DoLogText(const wxString& msg);

 };

#endif /* TRACKIRMOUSE_LOG_H */