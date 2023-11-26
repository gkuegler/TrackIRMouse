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
using GameTitleVector = std::vector<std::pair<std::string, std::string>>;
constexpr const std::array<std::string_view, 4> k_edge_names = { "left",
                                                                 "right",
                                                                 "top",
                                                                 "bottom" };

constexpr static const int LEFT_EDGE = 0;
constexpr static const int RIGHT_EDGE = 1;
constexpr static const int TOP_EDGE = 2;
constexpr static const int BOTTOM_EDGE = 3;

enum class retcode
{
  success,
  fail,
  track_ir_loss,
  graceful_exit
};

using HandleFunction = void (*)(Degrees, Degrees);

#endif /* TRACKIRMOUSE_TYPES_HPP */
