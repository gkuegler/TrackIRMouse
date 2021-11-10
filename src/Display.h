#ifndef TRACKIRMOUSE_DISPLAY_H
#define TRACKIRMOUSE_DISPLAY_H

class CDisplay {
public:

    // How to use:
    //   1. Set rotational bounds
    //   2. Set pixel bounds
    //   3. Call setAbsBounds()

    // User inputed rotational bounds of display
    float rotationBoundLeft = 0;
    float rotationBoundRight = 0;
    float rotationBoundTop = 0;
    float rotationBoundBottom = 0;

    float rotationBound16BitLeft = 0;
    float rotationBound16BitRight = 0;
    float rotationBound16BitTop = 0;
    float rotationBound16BitBottom = 0;

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
    float boundAbsLeft = 0;
    float boundAbsRight = 0;
    float boundAbsTop = 0;
    float boundAbsBottom = 0;

    // Ratio of input rotation to abolutized integer
    float ySlope{};
    float xSlope{};

    CDisplay(signed int left, signed int right, signed int top, signed int bottom)
    {
        pixelBoundLeft = left;
        pixelBoundRight = right;
        pixelBoundTop = top;
        pixelBoundBottom = bottom;
    }

    void setAbsBounds(signed int virtualOriginLeft, signed int virtualOriginTop,
        float x_PxToABS, float y_PxToABS) {
        pixelBoundAbsLeft = pixelBoundLeft - virtualOriginLeft;
        pixelBoundAbsRight = pixelBoundRight - virtualOriginLeft;
        pixelBoundAbsTop = pixelBoundTop - virtualOriginTop;
        pixelBoundAbsBottom = pixelBoundBottom - virtualOriginTop;

        boundAbsLeft = static_cast<float>(pixelBoundAbsLeft) * x_PxToABS;
        boundAbsRight = static_cast<float>(pixelBoundAbsRight) * x_PxToABS;
        boundAbsTop = pixelBoundAbsTop * y_PxToABS;
        boundAbsBottom = pixelBoundAbsBottom * y_PxToABS;

        rotationBound16BitLeft = rotationBoundLeft * (16383 / 180);
        rotationBound16BitRight = rotationBoundRight * (16383 / 180);
        rotationBound16BitTop = rotationBoundTop * (16383 / 180);
        rotationBound16BitBottom = rotationBoundBottom * (16383 / 180);

        float rl = rotationBound16BitLeft;
        float rr = rotationBound16BitRight;
        float al = boundAbsLeft;
        float ar = boundAbsRight;
        xSlope = (ar - al) / (rr - rl);

        float rt = rotationBound16BitTop;
        float rb = rotationBound16BitBottom;
        float at = boundAbsTop;
        float ab = boundAbsBottom;
        ySlope = -(at - ab) / (rt - rb);

        return;
    }
};

#endif /* TRACKIRMOUSE_DISPLAY_H */