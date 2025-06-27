#pragma once

#include <memory>

#include "display.hpp"
#include "mouse-modes.hpp"
#include "settings.hpp"
#include "types.hpp"

namespace handlers {
template<typename T>
class Coord
{
public:
  T yaw{ 0 };
  T pitch{ 0 };
};

// takes yaw and pitch information then converts it to mouse coordinates.
// will also move mouse
// TODO: change mouse handler to some sort of 'MouseStrategy' or
// 'CursorProtocol'?
class MouseHandler
{
private:
  std::shared_ptr<std::vector<Display>> displays_;
  std::atomic<bool> normal_mode_ = true;
  std::atomic<mouse_mode> mode_ = mouse_mode::scrollbar_right_small;
  Coord<Degrees> last_pos_{ 0.0, 0.0 };
  Degrees dead_zone_threshold_ = 0.05; // set to zero to disable dead zone

public:
  // TODO: Be more explicit in my data structure than the profile?
  MouseHandler(Profile profile);
  ~MouseHandler() {};

  void handle_input(const Degrees yaw, const Degrees pitch);
  void set_alternate_mode(mouse_mode mode);
  void toggle_alternate_mode() { normal_mode_ = !normal_mode_; };

private:
  // Move out of class definition into its own header file?
  void set_cursor_pos(double x, double y);
};
} // namespace handlers