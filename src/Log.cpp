#include "Log.h"

#define FMT_HEADER_ONLY
#include <fmt\xchar.h>
#include <wx/wx.h>

// ----------------------------------------------------------------------
// Functions Logging Specifically to the Text Control
// ----------------------------------------------------------------------

 void logToWix(std::string msg)
 {
    wxLogDebug(msg.c_str());
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
    wxLogDebug(msg);
 	wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD);
 	evt->SetString(wxString::wxString(msg));
 	wxTheApp->QueueEvent(evt);
 }

 void logToWix(const wchar_t* msg)
 {
 	wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD);
 	evt->SetString(wxString::wxString(msg));
 	wxTheApp->QueueEvent(evt);
 }

 // ----------------------------------------------------------------------
 // Functions for General-purpose Logging, to File, etc ...
 // ----------------------------------------------------------------------


 CMyLogger::CMyLogger()
{

	if (m_file.Exists(m_filename))
	{
		if (!m_file.Access(m_filename, wxFile::write))
		{
			wxLogFatalError("Unable to open log file.");
		}
	}

	m_file.Open(m_filename, wxFile::write);

}

void CMyLogger::DoLogText(const wxString& msg)
{
	bool success = m_file.Write(msg);

	if (!success)
	{
		wxLogError("Failed to Write to Log File.");
	}
}