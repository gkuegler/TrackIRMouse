#ifndef TRACKIRMOUSE_UTIL_H
#define TRACKIRMOUSE_UTIL_H

#include <spdlog/spdlog.h>
#include <wx/arrstr.h>

#include <vector>

namespace util {
// template <typename C<T>> requires ??

// container must be an irritable array of type T,
// where type T can be implicitly converted to wxString
template <typename T> 
wxArrayString BuildWxArrayString(const T container) {
  wxArrayString array;
  for (const auto& text : container) {
    array.Add(wxString(text), 1);
  }
  return array;
}
}  // namespace util
#endif /* TRACKIRMOUSE_UTIL_H */
