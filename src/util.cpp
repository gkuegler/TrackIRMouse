#include "util.hpp"

wxArrayString BuildArrayString(std::vector<std::string> container) {
  wxArrayString array;
  for (auto& text : container) {
    array.Add(wxString(text), 1);
  }
  return array;
}
