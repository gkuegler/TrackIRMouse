#include "Log.h"
#include <wx/wx.h>
#include <wx/string.h>
#include <string>

void logToWix(const char* msg)
{
	wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD);
	evt->SetString(wxString::FromAscii(msg));
	wxTheApp->QueueEvent(evt);
}

void logToWix(std::string msg)
{
	wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD);
	wxString mystring(msg.c_str(), wxConvUTF8);
	evt->SetString(mystring);
	wxTheApp->QueueEvent(evt);
}