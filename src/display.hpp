#ifndef TRACKIRMOUSE_DISPLAY_H
#define TRACKIRMOUSE_DISPLAY_H

#include "types.hpp"

typedef struct mouse_values_ {
  bool contains = false;
  double x = -1;
  double y = -1;
} CursorPosition;

class CDisplay {
 public:
  RectDegrees rotation_boundaries;  // User-specified
  RectPixels padding;               // padding
  RectPixels rel_edge_pixel;        // virtual desktop bounds
  RectShort coordinates;            // virtual desktop bounds

  // Ratio of input rotation to abolutized integer
  // used for linear interpolation
  double m_slope_coord_over_degrees_x{0.0};
  double m_slope_coord_over_degrees_y{0.0};
  double m_short_to_pixels_ratio_x{0.0};
  double m_short_to_pixels_ratio_y{0.0};

  CDisplay(const RectPixels pixel_edges, const RectDegrees rotation_values,
           const RectPixels padding_values) {
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
    coordinates[0] = static_cast<double>(rel_edge_pixel[0] - virtual_origin_left) * short_to_pixels_ratio_x;
    coordinates[1] = static_cast<double>(rel_edge_pixel[1] - virtual_origin_left) * short_to_pixels_ratio_x;
    coordinates[2] = static_cast<double>(rel_edge_pixel[2] - virtual_origin_top) * short_to_pixels_ratio_y;
    coordinates[3] = static_cast<double>(rel_edge_pixel[3] - virtual_origin_top) * short_to_pixels_ratio_y;

		//////////////////////////////////////////////////////////////////////
		//                      DIRECTION CONVENTIONS                       //
		//////////////////////////////////////////////////////////////////////

		// windows cursor left is pixels decreasing
    // trackir head left is yaw increaseing
		// windows cursor down is pixels is increasing
		// trackir head down is pitch increasing

    // setup linear interpolation parameters
    const auto rl = rotation_boundaries[LEFT_EDGE];  // left
    const auto rr = rotation_boundaries[RIGHT_EDGE];  // right
    const auto al = coordinates[LEFT_EDGE];
    const auto ar = coordinates[RIGHT_EDGE];
    m_slope_coord_over_degrees_x = std::abs(ar - al) / std::abs(rl - rr);

    const double rt = rotation_boundaries[TOP_EDGE];
    const double rb = rotation_boundaries[BOTTOM_EDGE];
    const double at = coordinates[TOP_EDGE];
    const double ab = coordinates[BOTTOM_EDGE];
    m_slope_coord_over_degrees_y = std::abs(at - ab) / std::abs(rb - rt);

    return;
  }
  // clang-format on
  const double get_horizontal_value(const Degrees yaw) {
    // linearly interpolate horizontal distance from left edge of display
    // note the direction sign conventions in above func
    return coordinates[LEFT_EDGE] + m_slope_coord_over_degrees_x *
                                        (rotation_boundaries[LEFT_EDGE] - yaw);
    ;
  }

  const double get_vertical_value(const Degrees pitch) {
    // linearly interpolate vertical distance from top edge of display
    // note the direction sign conventions in above func
    return coordinates[TOP_EDGE] + m_slope_coord_over_degrees_y *
                                       (pitch - rotation_boundaries[TOP_EDGE]);
  }

  const CursorPosition get_cursor_coordinates(const Degrees yaw,
                                              const Degrees pitch) {
    const double left = rotation_boundaries[LEFT_EDGE];
    const double right = rotation_boundaries[RIGHT_EDGE];
    const double top = rotation_boundaries[TOP_EDGE];
    const double bottom = rotation_boundaries[BOTTOM_EDGE];

    // test if degrees are within the displays mapping boundaries
    // note the direction sign conventions in above func
    if ((yaw < left) && (yaw > right) && (pitch > top) && (pitch < bottom)) {
      return {true, get_horizontal_value(yaw), get_vertical_value(pitch)};
    } else {
      return {false, -1, -1};
    }
  }

  const double get_inside_offset_from_edge(int edge, int pixels) {
    switch (edge) {
      case LEFT_EDGE:
        return coordinates[edge] + pixels * m_short_to_pixels_ratio_x;
      case RIGHT_EDGE:
        return coordinates[edge] - pixels * m_short_to_pixels_ratio_x;
      case TOP_EDGE:
        return coordinates[edge] + pixels * m_short_to_pixels_ratio_y;
      case BOTTOM_EDGE:
        return coordinates[edge] - pixels * m_short_to_pixels_ratio_y;
    }
  }

  const double get_padding_offset_from_edge(int edge) {
    switch (edge) {
      case LEFT_EDGE:
        return coordinates[edge] + padding[edge] * m_short_to_pixels_ratio_x;
      case RIGHT_EDGE:
        return coordinates[edge] - padding[edge] * m_short_to_pixels_ratio_x;
      case TOP_EDGE:
        return coordinates[edge] + padding[edge] * m_short_to_pixels_ratio_y;
      case BOTTOM_EDGE:
        return coordinates[edge] - padding[edge] * m_short_to_pixels_ratio_y;
    }
  }
};

#endif  // TRACKIRMOUSE_DISPLAY_H
