#ifndef TRACKIRMOUSE_HANDLERS_HPP
#define TRACKIRMOUSE_HANDLERS_HPP

#include <memory>

#include "display.hpp"
#include "mouse-modes.hpp"
#include "types.hpp"

namespace handlers {

// takes yaw and pitch information then converts it to mouse coordinates.
// will also move mouse
class MouseHandler
{
public:
  std::shared_ptr<std::vector<CDisplay>> displays_;
  std::atomic<bool> normal_mode_ = true;
  std::atomic<mouse_mode> mode_ = mouse_mode::scrollbar_right_small;

public:
  MouseHandler();
  ~MouseHandler(){};

  inline void set_cursor_pos(double x, double y);
  void handle_input(const Degrees yaw, const Degrees pitch);
  void set_alternate_mode(mouse_mode mode);
  void toggle_alternate_mode() { normal_mode_ = !normal_mode_; };
};
} // namespace handlers

#endif /* TRACKIRMOUSE_HANDLERS_HPP */
