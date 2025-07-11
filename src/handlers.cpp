#include "handlers.hpp"

#include "windows-wrapper.hpp"

#include <memory>

#include "display.hpp"
#include "environment.hpp"
#include "log.hpp"
#include "mouse-modes.hpp"
#include "settings.hpp"
#include "types.hpp"

namespace handlers {

#include <cmath> // Required for sqrt() and pow()

double
CalculateEuclideanDistance2D(double x1, double y1, double x2, double y2)
{
  double dx = x2 - x1;
  double dy = y2 - y1;
  return std::sqrt(std::pow(dx, 2) + std::pow(dy, 2));
}

MouseHandler::MouseHandler(settings::Profile profile)
{
  const auto info = WinDisplayInfo();

  auto hardware_display_count = info.rectangles.size();
  auto user_display_count = profile.displays.size();

  if (hardware_display_count > user_display_count) {
    // TODO: do wx log error here and pres okay to continue
    spdlog::error(
      "Warning: More displays found that were specified by the user. This "
      "should still work but will limit the mouse to only those displays "
      "specified.\n{} monitors specified but {} monitors found",
      user_display_count,
      hardware_display_count);
  } else if (hardware_display_count < user_display_count) {
    throw std::runtime_error(
      std::format("Incompatible user config: {} monitors specified but {} "
                  "monitors found",
                  user_display_count,
                  hardware_display_count));
  }

  std::vector<Display> displays;

  // Build display objects
  for (size_t i = 0; i < profile.displays.size(); i++) {
    auto& d = profile.displays[i];
    auto& rect = info.rectangles[i];
    // transfer config data to internal strucuture
    Display display(rect, d.rotation, d.padding);
    display.setAbsBounds(info.top_left_point.x,
                         info.top_left_point.y,
                         info.short_to_pixels_ratio_x,
                         info.short_to_pixels_ratio_y);
    displays.push_back(display);
  }

  displays_ = std::make_shared<std::vector<Display>>(displays);
}

inline void
MouseHandler::set_cursor_pos(double x, double y)
{
  static MOUSEINPUT mi = {
    0, 0, 0, MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK,
    0, 0
  };

  static INPUT ip = { INPUT_MOUSE, mi };

  ip.mi.dx = static_cast<LONG>(x);
  ip.mi.dy = static_cast<LONG>(y);

  if (0 == SendInput(1, &ip, sizeof(INPUT))) {
    spdlog::debug("SendInput was already blocked by another thread.");
  }

  return;
}

void
MouseHandler::handle_input(const Degrees yaw, const Degrees pitch)
{
  static int last_screen = 0;
  static double last_x = 0;
  constexpr static const double small_pix_offset = 5;    // pixels
  constexpr static const double minimap_pix_offset = 60; // pixels

  // Dead Zone Calculation
  // TODO: change to pitch and yaw
  auto d =
    CalculateEuclideanDistance2D(yaw, pitch, last_pos_.yaw, last_pos_.pitch);

  if (d < dead_zone_threshold_) {
    return;
  }

  last_pos_ = { yaw, pitch };

  if (normal_mode_) {
    // Check if the head is pointing to a screen
    // The return statement is never reached if the head is pointing outside
    // the bounds of any of the screense
    for (int i = 0; i < displays_->size(); i++) {
      auto pos = (*displays_)[i].get_cursor_coordinates(yaw, pitch);
      if (pos.contains) {
        set_cursor_pos(pos.x, pos.y);
        last_screen = i;
        last_x = pos.x;
        return; // head is pointing within display, move mouse and return
      }
    }
    // head did not point within any particular display, move on to next
    // section
  }
  // If the head is pointing outside of the bounds of a screen, the mouse
  // should snap to the breached edge. It could either be the pitch or the
  // yaw axis that is too great or too little.
  // Assume the pointer came from the last screen, just asign the mouse
  // position to the absolute limit from the screen it came from.
  double x;
  double y;

  Display& dlast = (*displays_)[last_screen];
  const double left = dlast.rotation_boundaries[LEFT_EDGE];
  const double right = dlast.rotation_boundaries[RIGHT_EDGE];
  const double top = dlast.rotation_boundaries[TOP_EDGE];
  const double bottom = dlast.rotation_boundaries[BOTTOM_EDGE];

  if (yaw >= left) { // yaw is left of last used display
    x = dlast.get_padding_offset_from_edge(LEFT_EDGE);
  } else if (yaw <= right) { // yaw is right of last used display
    x = dlast.get_padding_offset_from_edge(RIGHT_EDGE);
  } else { // yaw within last display boundaries as normal
    x = dlast.get_horizontal_value(yaw);
  }

  if (pitch <= top) { // pitch is above last used display
    y = dlast.get_padding_offset_from_edge(TOP_EDGE);
  } else if (pitch >= bottom) { // pitch is below last used display
    y = dlast.get_padding_offset_from_edge(BOTTOM_EDGE);
  } else { // pitch within last display boundaries as normal
    y = dlast.get_vertical_value(pitch);
  }

  if (false == normal_mode_) {
    switch (mode_) {
      case mouse_mode::scrollbar_left_small:
        x = dlast.get_inside_offset_from_edge(LEFT_EDGE, small_pix_offset);
        break;
      case mouse_mode::scrollbar_left_mini_map:
        x = dlast.get_inside_offset_from_edge(LEFT_EDGE, minimap_pix_offset);
        break;
      case mouse_mode::scrollbar_right_small:
        x = dlast.get_inside_offset_from_edge(RIGHT_EDGE, small_pix_offset);
        break;
      case mouse_mode::scrollbar_right_mini_map:
        x = dlast.get_inside_offset_from_edge(RIGHT_EDGE, minimap_pix_offset);
        break;
      case mouse_mode::scrollbar_hold_x:
        x = last_x;
        break;
      case mouse_mode::autocad_zoom:
        return;
      default:
        break;
    }
  }

  set_cursor_pos(x, y);
  return;
}

void
MouseHandler::set_alternate_mode(mouse_mode mode)
{
  mode_ = mode;
};

} // namespace handlers
