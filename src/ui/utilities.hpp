#pragma once

#include <wx/arrstr.h>

namespace utility {
// container must be an irritable array of type T,
// where type T can be implicitly converted to wxString
template<typename T>
wxArrayString
BuildWxArrayString(const T container)
{
  wxArrayString array;
  for (const auto& text : container) {
    array.Add(wxString(text), 1);
  }
  return array;
}
}