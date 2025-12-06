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

/*
 * Handles head tracking data into mouse position.
 */
class MouseHandler
{
private:
  Profile profile;
  std::shared_ptr<std::vector<Display>> displays;
  std::atomic<bool> is_normal_mode = true;
  std::atomic<mouse_mode> alt_mode = mouse_mode::scrollbar_right_small;
  Coord<Degrees> last_pos{ 0.0, 0.0 };
  Point<long> last_pos_px{ 0, 0 };

public:
  MouseHandler(Profile);
  ~MouseHandler() {};

  void HandleInput(const Degrees yaw, const Degrees pitch);
  void SetAlternateMode(mouse_mode mode);
  void toggle_alternate_mode() { is_normal_mode = !is_normal_mode; };

private:
  // Move out of class definition into its own header file?
  void SetCursorPosition(double x, double y);
};
} // namespace handlers
