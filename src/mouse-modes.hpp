#ifndef TIRMOUSE_MOUSE_MODES_HPP
#define TIRMOUSE_MOUSE_MODES_HPP

// #include <map>
#include <wx/string.h>

enum class mouse_mode
{
  move_mouse,
  previous,
  scrollbar_left_small,
  scrollbar_left_mini_map,
  scrollbar_right_small,
  scrollbar_right_mini_map,
  scrollbar_hold_x,
  autocad_zoom,
};

// for future use
// static std::map<std::string, enum mouse_mode> available_alternate_modes = {
//  { "scrollbar_left_small", mouse_mode::scrollbar_left_small },
//  { "scrollbar_left_mini_map", mouse_mode::scrollbar_left_mini_map },
//  { "scrollbar_right_small", mouse_mode::scrollbar_right_small },
//  { "scrollbar_right_mini_map", mouse_mode::scrollbar_right_mini_map },
//  { "scrollbar_hold_x", mouse_mode::scrollbar_hold_x },
//  { "autocad_zoom", mouse_mode::autocad_zoom }
//};

mouse_mode
GetModeByExecutableName(wxString name);
void
UpdateModesbyExecutableName(wxString name, mouse_mode mode);
#endif /* TIRMOUSE_MOUSE_MODES_HPP */
