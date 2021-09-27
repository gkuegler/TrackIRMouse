#pragma once
#ifndef TRACKIRMOUSE_LOG_H
#define TRACKIRMOUSE_LOG_H

#include <string>

//template <class T>
//void logToWix(T msg)
//{
//	wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD);
//	evt->SetString(wxString::FromAscii(msg));
//	wxTheApp->QueueEvent(evt);
//}

void logToWix(std::string msg);
void logToWix(std::wstring msg);

void logToWix(const char* msg);
void logToWix(const wchar_t* msg);

#endif /* TRACKIRMOUSE_LOG_H */