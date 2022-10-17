#ifndef TRACKIRMOUSE_TYPES_HPP
#define TRACKIRMOUSE_TYPES_HPP

#include <array>
#include <string>
#include <vector>

using Degrees = double;
using Pixels = signed int;
using RectDegrees = std::array<Degrees, 4>;
using RectPixels = std::array<Pixels, 4>;
using RectShort = std::array<double, 4>;
using GameTitles = std::vector<std::pair<std::string, std::string>>;

constexpr static const int LEFT_EDGE = 0;
constexpr static const int RIGHT_EDGE = 1;
constexpr static const int TOP_EDGE = 2;
constexpr static const int BOTTOM_EDGE = 3;

enum class retcode
{
  success,
  fail
};

enum class msgcode
{
  normal,
  log_normal_text,
  log_red_text,
  toggle_tracking,
  set_mode,
  close_app
};

enum class mouse_mode
{
  move_mouse,
  scrollbar_left_small,
  scrollbar_left_mini_map,
  scrollbar_right_small,
  scrollbar_right_mini_map,
  scrollbar_hold_x,
  autocad_zoom,
};

using HandleFunction = void (*)(Degrees, Degrees);

#endif /* TRACKIRMOUSE_TYPES_HPP */
