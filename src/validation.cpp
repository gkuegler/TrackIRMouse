#include "validation.hpp"

#include "log.hpp"

namespace validation {

bool
ValidateUserInput(const UserInput& displays)
{
  // compare bounds not more than abs|180|
  for (int i = 0; i < displays.size(); i++) {
    for (int j = 0; j < 4; j++) {
      auto d = displays[i].rotation[j];
      auto padding = displays[i].padding[j];
      if (d > 180.0 || d < -180.0) {
        spdlog::error("rotation bound param \"{}\" on display #{} is outside "
                      "allowable range of -180° -> 180°.",
                      k_edge_names[j],
                      i);
        return false;
      }
    }
  }
  // see if any rectangles overlap
  // visualization: https://silentmatt.com/rectangle-intersection/
  for (int i = 0; i < displays.size(); i++) {
    int j = i;
    while (++j < displays.size()) {
      Degrees A_left = displays[i].rotation[LEFT_EDGE];
      Degrees A_right = displays[i].rotation[RIGHT_EDGE];
      Degrees A_top = displays[i].rotation[TOP_EDGE];
      Degrees A_bottom = displays[i].rotation[BOTTOM_EDGE];
      Degrees B_left = displays[j].rotation[LEFT_EDGE];
      Degrees B_right = displays[j].rotation[RIGHT_EDGE];
      Degrees B_top = displays[j].rotation[TOP_EDGE];
      Degrees B_bottom = displays[j].rotation[BOTTOM_EDGE];
      if (A_left < B_right && A_right > B_left && A_top > B_bottom &&
          A_bottom < B_top) {
        spdlog::error(
          "Overlapping rotational bounds between display #{} and #{}", i, j);
        return false;
      }
    }
  }
  spdlog::trace("user input validated");
  return true;
}
}
