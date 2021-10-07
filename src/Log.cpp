#include "Log.h"

#include <wx/wx.h>
#include <wx/string.h>
#include <fmt\xchar.h>
//#include <spdlog/spdlog.h>

#include <string>

 void logToWix(std::string msg)
 {
 	wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD);
 	wxString mystring(msg.c_str(), wxConvUTF8);
 	evt->SetString(mystring);
 	wxTheApp->QueueEvent(evt);
 }

 void logToWix(std::wstring msg)
 {
 	wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD);
 	wxString mystring(msg.c_str(), wxConvUTF8);
 	evt->SetString(mystring);
 	wxTheApp->QueueEvent(evt);
 }

 void logToWix(const char* msg)
 {
 	wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD);
 	//evt->SetString(wxString::FromAscii(msg));
 	evt->SetString(wxString::wxString(msg));
 	//evt->SetString(wxString(msg));
 	wxTheApp->QueueEvent(evt);
 }

 void logToWix(const wchar_t* msg)
 {
 	wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD);
 	evt->SetString(wxString::wxString(msg));
 	wxTheApp->QueueEvent(evt);
 }