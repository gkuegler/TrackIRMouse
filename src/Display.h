class CDisplay {
public:

    // How to use:
    //   1. Set rotational bounds
    //   2. Set pixel bounds
    //   3. Call setAbsBounds()

    // User inputed rotational bounds of display
    float rot_left = 0;
    float rot_right = 0;
    float rot_top = 0;
    float rot_bottom = 0;

    float rot_s15bit_left = 0;
    float rot_s15bit_right = 0;
    float rot_s15bit_top = 0;
    float rot_s15bit_bottom = 0;

    // User-specified
    int left_padding = 3;
    int right_padding = 3;
    int top_padding = 0;
    int bottom_padding = 0;

    // Virtual desktop bounds of display relative to main monitor
    signed int pix_left = 0;
    signed int pix_right = 0;
    signed int pix_top = 0;
    signed int pix_bottom = 0;

    // Resulting Virtualized virtual desktop bounds
    signed int pix_abs_left = 0;
    signed int pix_abs_right = 0;
    signed int pix_abs_top = 0;
    signed int pix_abs_bottom = 0;

    // Resulting Mapped bounds of display in absolute from top left most display
    float abs_left = 0;
    float abs_right = 0;
    float abs_top = 0;
    float abs_bottom = 0;

    // Ratio of input rotation to abolutized integer
    float ySlope{};
    float xSlope{};

    void setAbsBounds(signed int virt_origin_left, signed int virt_origin_top,
        float x_PxToABS, float y_PxToABS) {
        pix_abs_left = pix_left - virt_origin_left;
        pix_abs_right = pix_right - virt_origin_left;
        pix_abs_top = pix_top - virt_origin_top;
        pix_abs_bottom = pix_bottom - virt_origin_top;

        abs_left = static_cast<float>(pix_abs_left) * x_PxToABS;
        abs_right = static_cast<float>(pix_abs_right) * x_PxToABS;
        abs_top = pix_abs_top * y_PxToABS;
        abs_bottom = pix_abs_bottom * y_PxToABS;

        rot_s15bit_left = rot_left * (16383 / 180);
        rot_s15bit_right = rot_right * (16383 / 180);
        rot_s15bit_top = rot_top * (16383 / 180);
        rot_s15bit_bottom = rot_bottom * (16383 / 180);

        float rl = rot_s15bit_left;
        float rr = rot_s15bit_right;
        float al = abs_left;
        float ar = abs_right;
        xSlope = (ar - al) / (rr - rl);

        float rt = rot_s15bit_top;
        float rb = rot_s15bit_bottom;
        float at = abs_top;
        float ab = abs_bottom;
        ySlope = -(at - ab) / (rt - rb);

        return;
    }
};