#pragma once

#include "types.hpp"

struct CursorCoordinates
{
  bool contains = false;
  double x = -1;
  double y = -1;
};

class Display
{
public:
  RectDegrees rotation_boundaries; // User-specified
  RectPixels padding;              // padding
  RectPixels rel_edge_pixel;       // virtual desktop bounds
  RectShort coordinates;           // virtual desktop bounds

  // Ratio of input rotation to abolutized integer
  // used for linear interpolation
  double m_slope_coord_over_degrees_x{ 0.0 };
  double m_slope_coord_over_degrees_y{ 0.0 };
  double m_short_to_pixels_ratio_x{ 0.0 };
  double m_short_to_pixels_ratio_y{ 0.0 };

  Display(const RectPixels pixel_edges,
          const RectDegrees rotation_values,
          const RectPixels padding_values)
  {
    rel_edge_pixel = pixel_edges;
    rotation_boundaries = rotation_values;
    padding = padding_values;
  }
  // clang-format off
  void setAbsBounds(signed int virtual_origin_left,
                    signed int virtual_origin_top,
                    double short_to_pixels_ratio_x,
                    double short_to_pixels_ratio_y) {
		m_short_to_pixels_ratio_x = short_to_pixels_ratio_x;
		m_short_to_pixels_ratio_y = short_to_pixels_ratio_y;
    // Maps user defined roational bounds (representing the direction of head
    // pointing) to the boundaries of a display. Uses linear interpolation.
    // SendInput for mouse accepts an unsigned 16bit input representing the
    // virtual desktop area with (0, 0) starting at the top left most monitor.
    // Th windows api querries to obtain the RECT struct or each monitor
    // return values relative to the main display. Transformation is as
    // follows: virtual pixel bounds (with origin at main display) -> absolute
    // left right top bottom
		// 
    // find the absolute short_int value of a display edge
    coordinates.left = static_cast<double>(rel_edge_pixel.left - virtual_origin_left) * short_to_pixels_ratio_x;
    coordinates.right = static_cast<double>(rel_edge_pixel.right - virtual_origin_left) * short_to_pixels_ratio_x;
    coordinates.top = static_cast<double>(rel_edge_pixel.top - virtual_origin_top) * short_to_pixels_ratio_y;
    coordinates.bottom = static_cast<double>(rel_edge_pixel.bottom - virtual_origin_top) * short_to_pixels_ratio_y;

		//////////////////////////////////////////////////////////////////////
		//                      DIRECTION CONVENTIONS                       //
		//////////////////////////////////////////////////////////////////////

		// windows cursor left is pixels decreasing
    // trackir head left is yaw increaseing
		// windows cursor down is pixels is increasing
		// trackir head down is pitch increasing

    // setup linear interpolation parameters
    const auto rl = rotation_boundaries.left;  // left
    const auto rr = rotation_boundaries.right;  // right
    const auto al = coordinates.left;
    const auto ar = coordinates.right;
    m_slope_coord_over_degrees_x = std::abs(ar - al) / std::abs(rl - rr);

    const double rt = rotation_boundaries.top;
    const double rb = rotation_boundaries.bottom;
    const double at = coordinates.top;
    const double ab = coordinates.bottom;
    m_slope_coord_over_degrees_y = std::abs(at - ab) / std::abs(rb - rt);

    return;
  }
  // clang-format on
  const double get_horizontal_value(const Degrees yaw)
  {
    // linearly interpolate horizontal distance from left edge of display
    // note the direction sign conventions in above func
    return coordinates.left + m_slope_coord_over_degrees_x * (rotation_boundaries.left - yaw);
    ;
  }

  const double get_vertical_value(const Degrees pitch)
  {
    // linearly interpolate vertical distance from top edge of display
    // note the direction sign conventions in above func
    return coordinates.top + m_slope_coord_over_degrees_y * (pitch - rotation_boundaries.top);
  }

  const CursorCoordinates get_cursor_coordinates(const Degrees yaw, const Degrees pitch)
  {
    const double left = rotation_boundaries.left;
    const double right = rotation_boundaries.right;
    const double top = rotation_boundaries.top;
    const double bottom = rotation_boundaries.bottom;

    // test if degrees are within the displays mapping boundaries
    // note the direction sign conventions in above func
    if ((yaw < left) && (yaw > right) && (pitch > top) && (pitch < bottom)) {
      return { true, get_horizontal_value(yaw), get_vertical_value(pitch) };
    } else {
      return {};
    }
  }

  // TODO: can i make this return all of the edges or return a coord instead?
  const double get_inside_offset_from_edge(int edge, int pixels)
  {
    switch (edge) {
      case LEFT_EDGE:
        return coordinates.left + pixels * m_short_to_pixels_ratio_x;
      case RIGHT_EDGE:
        return coordinates.right - pixels * m_short_to_pixels_ratio_x;
      case TOP_EDGE:
        return coordinates.top + pixels * m_short_to_pixels_ratio_y;
      case BOTTOM_EDGE:
        return coordinates.bottom - pixels * m_short_to_pixels_ratio_y;
    }
  }

  const double get_padding_offset_from_edge(int edge)
  {
    switch (edge) {
      case LEFT_EDGE:
        return coordinates.left + padding.left * m_short_to_pixels_ratio_x;
      case RIGHT_EDGE:
        return coordinates.right - padding.right * m_short_to_pixels_ratio_x;
      case TOP_EDGE:
        return coordinates.top + padding.top * m_short_to_pixels_ratio_y;
      case BOTTOM_EDGE:
        return coordinates.bottom - padding.bottom * m_short_to_pixels_ratio_y;
    }
  }
};
