#ifndef TRACKIRMOUSE_CONFIG_H
#define TRACKIRMOUSE_CONFIG_H

#include <string>
#include "Constants.h"

struct bounds_in_degrees {
    float left = 0;
    float right = 0;
    float top = 0;
    float bottom = 0;

    //These are actually in pixels
    int pad_left = 0;
    int pad_right = 0;
    int pad_top = 0;
    int pad_bottom = 0;
};

class CConfig {
public:

    int profile_ID = 0;
    bool bWatchdog = 0;
    std::string sTrackIR_dll_location;
    int display_profile = 0;

    bounds_in_degrees bounds[DEFAULT_MAX_DISPLAYS];

    CConfig() {};
    void LoadSettings(int);


};

#endif /* TRACKIRMOUSE_CONFIG_H */