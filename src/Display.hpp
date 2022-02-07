#ifndef TRACKIRMOUSE_DISPLAY_H
#define TRACKIRMOUSE_DISPLAY_H

class CDisplay {
 public:
  // How to use:
  //   1. Set rotational bounds
  //   2. Set pixel bounds
  //   3. Call setAbsBounds()

  // User inputed rotational bounds of display
  double rotationBoundLeft = 0;
  double rotationBoundRight = 0;
  double rotationBoundTop = 0;
  double rotationBoundBottom = 0;

  double rotationBound16BitLeft = 0;
  double rotationBound16BitRight = 0;
  double rotationBound16BitTop = 0;
  double rotationBound16BitBottom = 0;

  // User-specified
  int paddingLeft = 3;
  int paddingRight = 3;
  int paddingTop = 0;
  int paddingBottom = 0;

  // Virtual desktop bounds of display relative to main monitor
  signed int pixelBoundLeft = 0;
  signed int pixelBoundRight = 0;
  signed int pixelBoundTop = 0;
  signed int pixelBoundBottom = 0;

  // Resulting Virtualized virtual desktop bounds
  signed int pixelBoundAbsLeft = 0;
  signed int pixelBoundAbsRight = 0;
  signed int pixelBoundAbsTop = 0;
  signed int pixelBoundAbsBottom = 0;

  // Resulting Mapped bounds of display in absolute from top left most display
  double boundAbsLeft = 0;
  double boundAbsRight = 0;
  double boundAbsTop = 0;
  double boundAbsBottom = 0;

  // Ratio of input rotation to abolutized integer
  // used for linear interpolation
  double ySlope{};
  double xSlope{};

  CDisplay(signed int left, signed int right, signed int top,
           signed int bottom) {
    pixelBoundLeft = left;
    pixelBoundRight = right;
    pixelBoundTop = top;
    pixelBoundBottom = bottom;
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
    pixelBoundAbsLeft = pixelBoundLeft - virtualOriginLeft;
    pixelBoundAbsRight = pixelBoundRight - virtualOriginLeft;
    pixelBoundAbsTop = pixelBoundTop - virtualOriginTop;
    pixelBoundAbsBottom = pixelBoundBottom - virtualOriginTop;

    boundAbsLeft = static_cast<double>(pixelBoundAbsLeft) * x_PxToABS;
    boundAbsRight = static_cast<double>(pixelBoundAbsRight) * x_PxToABS;
    boundAbsTop = pixelBoundAbsTop * y_PxToABS;
    boundAbsBottom = pixelBoundAbsBottom * y_PxToABS;

    // convert to 16bit values because natural point software
    // gives head tracking data in 16bit values.
    // mapping the values to 16bit now saves and extra conversion
    // step later
    rotationBound16BitLeft = rotationBoundLeft * (16383 / 180);
    rotationBound16BitRight = rotationBoundRight * (16383 / 180);
    rotationBound16BitTop = rotationBoundTop * (16383 / 180);
    rotationBound16BitBottom = rotationBoundBottom * (16383 / 180);

    // setup linear interpolation parameters
    double rl = rotationBound16BitLeft;
    double rr = rotationBound16BitRight;
    double al = boundAbsLeft;
    double ar = boundAbsRight;
    xSlope = (ar - al) / (rr - rl);

    double rt = rotationBound16BitTop;
    double rb = rotationBound16BitBottom;
    double at = boundAbsTop;
    double ab = boundAbsBottom;
    ySlope = -(at - ab) / (rt - rb);

    return;
  }
};

#endif /* TRACKIRMOUSE_DISPLAY_H */
