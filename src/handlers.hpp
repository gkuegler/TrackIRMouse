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
  mouse_mode m_mode = mouse_mode::move_mouse;

  MouseHandler();
  ~MouseHandler(){};

  inline void send_my_input(double x, double y);
  void handle_input(const Degrees yaw, const Degrees pitch);
};
}  // namespace handlers

#endif /* TRACKIRMOUSE_HANDLERS_HPP */
