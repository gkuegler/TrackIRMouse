#include "messages.hpp"

#include <wx/log.h>
#include <wx/wx.h>

#include "types.hpp"

void
SendThreadMessage(msgcode code, std::string msg)
{
  wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD);
  event->SetString(wxString(msg));
  event->SetInt(static_cast<int>(code));
  wxTheApp->QueueEvent(event);
}

void
SendThreadMessage(msgcode code, std::string msg, long optional_param)
{
  wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD);
  event->SetInt(static_cast<int>(code));
  event->SetString(wxString(msg));
  event->SetExtraLong(optional_param);
  wxTheApp->QueueEvent(event);
}

void
SendThreadMessage(msgcode code, long param)
{
  wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD);
  event->SetInt(static_cast<int>(code));
  event->SetExtraLong(param);
  wxTheApp->QueueEvent(event);
}