#include "mouse-modes.hpp"
#include <map>
#include <mutex>

static std::map<wxString, enum mouse_mode> g_modes;
static std::mutex g_modes_mutex;

// put in Toml file
// [ScrollModesByApp]
// scroll_left_mini_map = ["sublime_text"]

mouse_mode
GetModeByExecutableName(wxString name)
{
  std::lock_guard<std::mutex> guard(g_modes_mutex);
  try {
    return g_modes.at(name);
  } catch (const std::out_of_range&) {
    return mouse_mode::scrollbar_right_small;
  }
}

void
UpdateModesbyExecutableName(wxString name, mouse_mode mode)
{
  std::lock_guard<std::mutex> guard(g_modes_mutex);
  g_modes[name] = mode;
}
