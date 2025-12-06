#pragma once

// #include <map>
#include <wx/string.h>

// TODO: make snaps adjustable in settings.
/*
 * move_mouse:
 *   Standard mode to control mouse.
 * previous:
 * scrollbar_left_small:
 *   Snap the mouse to just inside left edge of the screen.
 * scrollbar_left_mini_map:
 *   Snap the mouse to a little further in from the left edge of the screen.
 * scrollbar_right_small:
 * scrollbar_right_mini_map:
 * scrollbar_hold_x:
 *   Lock the current horizontal position of the mouse.
 * autocad_zoom:
 */
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

// TODO: put in help?
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