#ifndef TRACKIRMOUSE_DISPLAY_H
#define TRACKIRMOUSE_DISPLAY_H

#include "types.hpp"

typedef struct mouse_values_ {
  bool contains = false;
  double x = -1;
  double y = -1;
} MousePosition;

class CDisplay {
 public:
  // How to use:
  //   1. Set pixel bounds by windows call
  //   2. Set rotational bounds from config active profile
  //   3. Call setAbsBounds()

  std::array<double, 4> rotation;       // User-specified
  std::array<double, 4> rotation16bit;  // Virtual desktop bounds of
                                        // display relative to main monitor
  std::array<int, 4> padding;           // padding
  std::array<signed int, 4> relPixel;   // Virtual desktop bounds of
                                        // display relative to main monitor
  std::array<signed int, 4> absPixel;   // Resulting Virtualized virtual
                                        // desktop bounds
  std::array<double, 4> absCached;      // Resulting Mapped bounds of display
                                        // in absolute from top left most
                                        // display

  // Ratio of input rotation to abolutized integer
  // used for linear interpolation
  double ySlope{0.0};
  double xSlope{0.0};

  // interface used by WindowsSetup
  CDisplay(signed int left, signed int right, signed int top,
           signed int bottom) {
    relPixel[0] = left;
    relPixel[1] = right;
    relPixel[2] = top;
    relPixel[3] = bottom;
  }

  void setAbsBounds(signed int virtualOriginLeft, signed int virtualOriginTop,
                    double x_PxToABS, double y_PxToABS) {
    // Maps user defined roational bounds (representing the direction of head
    // pointing) to the boundaries of a display. Uses linear interpolation.
    // SendInput for mouse accepts an unsigned 16bit input representing the
    // virtual desktop area with (0, 0) starting at the top left most monitor.
    // Th windows api querries to obtain the RECT struct or each monitor return
    // values relative to the main display.
    // Transformation is as follows:
    // virtual pixel bounds (with origin at main display) -> absolute
    // left right top bottom
    absPixel[0] = relPixel[0] - virtualOriginLeft;  // left
    absPixel[1] = relPixel[1] - virtualOriginLeft;  // right
    absPixel[2] = relPixel[2] - virtualOriginTop;   // top
    absPixel[3] = relPixel[3] - virtualOriginTop;   // bottom

    absCached[0] = static_cast<double>(absPixel[0]) * x_PxToABS;
    absCached[1] = static_cast<double>(absPixel[1]) * x_PxToABS;
    absCached[2] = static_cast<double>(absPixel[2]) * y_PxToABS;
    absCached[3] = static_cast<double>(absPixel[3]) * y_PxToABS;

    // convert to 16bit values because natural point software
    // gives head tracking data in 16bit values.
    // mapping the values to 16bit now saves and extra conversion
    // step later
    rotation16bit[0] = rotation[0] * (16383 / 180);
    rotation16bit[1] = rotation[1] * (16383 / 180);
    rotation16bit[2] = rotation[2] * (16383 / 180);
    rotation16bit[3] = rotation[3] * (16383 / 180);

    // setup linear interpolation parameters
    double rl = rotation16bit[0];  // left
    double rr = rotation16bit[1];  // right
    double al = absCached[0];
    double ar = absCached[1];
    xSlope = (ar - al) / (rr - rl);

    double rt = rotation16bit[2];
    double rb = rotation16bit[3];
    double at = absCached[2];
    double ab = absCached[3];
    ySlope = -(at - ab) / (rt - rb);

    return;
  }

  const double getHorizontalPosition(const Deg yaw) {
    // interpolate horizontal position from left edge of display
    return xSlope * (yaw - rotation16bit[0]) + absCached[0];  // x
  }

  const double getVerticalPosition(const Deg pitch) {
    // interpolate vertical position from top edge of display
    return ySlope * (rotation16bit[2] - pitch) + absCached[2];  // y
  }

  const MousePosition getMousePosition(const Deg yaw, const Deg pitch) {
    const double rl = rotation16bit[0];
    const double rr = rotation16bit[1];
    const double rt = rotation16bit[2];
    const double rb = rotation16bit[3];
    if ((yaw > rl) && (yaw < rr) && (pitch < rt) && (pitch > rb)) {
      return {true, getHorizontalPosition(yaw), getVerticalPosition(pitch)};
    } else {
      return {false, -1, -1};
    }
  }
};

#endif /* TRACKIRMOUSE_DISPLAY_H */
