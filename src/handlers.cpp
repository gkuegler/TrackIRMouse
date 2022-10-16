

#include "handlers.hpp"

#include <windows.h>

#include <memory>

#include "config.hpp"
#include "display.hpp"
#include "log.hpp"
#include "types.hpp"
#include "wdi.hpp"

namespace handlers {

MouseHandler::MouseHandler() {
  const auto profile = config::Get()->GetActiveProfile();
  const auto info = environment::GetWindowsMonitorInformation();

  if (info.rectangles.size() != profile.displays.size()) {
    throw std::runtime_error(
        std::format("Incompatible config: {} monitors specified but {} "
                    "monitors found",
                    profile.displays.size(), info.rectangles.size()));
  }

  std::vector<CDisplay> displays;

  // Build display objects
  for (size_t i = 0; i < profile.displays.size(); i++) {
    auto &d = profile.displays[i];
    auto &rect = info.rectangles[i];

    // transfer config data to internal strucuture
    CDisplay display(rect, d.rotation, d.padding);
    display.setAbsBounds(info.origin_offset_x, info.origin_offset_y,
                         info.short_to_pixels_ratio_x,
                         info.short_to_pixels_ratio_y);
    displays.push_back(display);
  }

  m_displays = std::make_shared<std::vector<CDisplay>>(displays);
}

inline void MouseHandler::send_my_input(double x, double y) {
  static MOUSEINPUT mi = {
      0, 0,
      0, MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK,
      0, 0};

  static INPUT ip = {INPUT_MOUSE, mi};

  ip.mi.dx = static_cast<LONG>(x);
  ip.mi.dy = static_cast<LONG>(y);

  if (0 == SendInput(1, &ip, sizeof(INPUT))) {
    spdlog::debug("SendInput was already blocked by another thread.");
  }

  return;
}

void MouseHandler::handle_input(const Degrees yaw, const Degrees pitch) {
  static int last_screen = 0;
  static double last_x = 0;
  constexpr static const double small_pix_offset = 5;     // pixels
  constexpr static const double minimap_pix_offset = 60;  // pixels

  if (m_normal_mode) {
    // Check if the head is pointing to a screen
    // The return statement is never reached if the head is pointing outside
    // the bounds of any of the screense
    for (int i = 0; i < m_displays->size(); i++) {
      auto pos = (*m_displays)[i].get_cursor_coordinates(yaw, pitch);
      if (pos.contains) {
        send_my_input(pos.x, pos.y);
        last_screen = i;
        last_x = pos.x;
        return;  // head is pointing within display, move mouse and return
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

  CDisplay &dlast = (*m_displays)[last_screen];
  const double left = dlast.rotation_boundaries[LEFT_EDGE];
  const double right = dlast.rotation_boundaries[RIGHT_EDGE];
  const double top = dlast.rotation_boundaries[TOP_EDGE];
  const double bottom = dlast.rotation_boundaries[BOTTOM_EDGE];

  if (yaw >= left) {  // yaw is left of last used display
    x = dlast.get_padding_offset_from_edge(LEFT_EDGE);
  } else if (yaw <= right) {  // yaw is right of last used display
    x = dlast.get_padding_offset_from_edge(RIGHT_EDGE);
  } else {  // yaw within last display boundaries as normal
    x = dlast.get_horizontal_value(yaw);
  }

  if (pitch <= top) {  // pitch is above last used display
    y = dlast.get_padding_offset_from_edge(TOP_EDGE);
  } else if (pitch >= bottom) {  // pitch is below last used display
    y = dlast.get_padding_offset_from_edge(BOTTOM_EDGE);
  } else {  // pitch within last display boundaries as normal
    y = dlast.get_vertical_value(pitch);
  }

  if (false == m_normal_mode) {
    switch (m_mode) {
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

  send_my_input(x, y);
  return;
}

}  // namespace handlers
