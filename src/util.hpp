#ifndef TRACKIRMOUSE_UTIL_H
#define TRACKIRMOUSE_UTIL_H

#include <spdlog/spdlog.h>
#include <wx/arrstr.h>

#include <vector>

template <typename T>
wxArrayString BuildArrayString(const T container) {
  wxArrayString array;
  for (const auto& text : container) {
    spdlog::info("text -> {}", text);
    array.Add(wxString(text), 1);
  }
  return array;
}
#endif /* TRACKIRMOUSE_UTIL_H */
