#ifndef TRACKIRMOUSE_HANDLERS_HPP
#define TRACKIRMOUSE_HANDLERS_HPP

#include <memory>

#include "display.hpp"
#include "types.hpp"

namespace handlers {

// takes yaw and pitch information then converts it to mouse coordinates.
// will also move mouse
class MouseHandler {
 public:
  std::shared_ptr<std::vector<CDisplay>> m_displays;
  std::atomic<bool> m_normal_mode = true;
  // TODO: make a persistent scroll mode for each monitor
  std::atomic<mouse_mode> m_mode = mouse_mode::scrollbar_right_mini_map;

  MouseHandler();
  ~MouseHandler(){};

  inline void send_my_input(double x, double y);
  void handle_input(const Degrees yaw, const Degrees pitch);
  void set_alternate_mode(mouse_mode mode) { m_mode = mode; };
  void toggle_alternate_mode() { m_normal_mode = !m_normal_mode; };
};
}  // namespace handlers

#endif /* TRACKIRMOUSE_HANDLERS_HPP */
