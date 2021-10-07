#pragma once
#ifndef TRACKIRMOUSE_LOG_H
#define TRACKIRMOUSE_LOG_H

//#include <fmt\xchar.h>
//#include <wx/wx.h>
//#include <wx/string.h>
//#include <spdlog/spdlog.h>

#include <string>


 void logToWix(std::string msg);
 void logToWix(std::wstring msg);

 void logToWix(const char* msg);
 void logToWix(const wchar_t* msg);

#endif /* TRACKIRMOUSE_LOG_H */