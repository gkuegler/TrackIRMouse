#include "messages.hpp"

#include <wx/log.h>
#include <wx/wx.h>

#include "types.hpp"

void
SendThreadMessage(msgcode code, std::string msg, long optional_param)
{
  wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD);
  event->SetInt(static_cast<int>(code));
  event->SetString(wxString(msg));
  event->SetExtraLong(static_cast<long>(optional_param));
  wxTheApp->QueueEvent(event);
}

void
SendThreadMessage(msgcode code, std::string msg)
{
  wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD);
  event->SetString(wxString(msg));
  event->SetInt(static_cast<long>(code));
  wxTheApp->QueueEvent(event);
}
