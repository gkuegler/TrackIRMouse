#ifndef TRACKIRMOUSE_CONFIG_H
#define TRACKIRMOUSE_CONFIG_H

#include <string>
#include "Constants.h"


// class CConfig
// {
// public:
//     int profile_ID;

//     bool bWatchdog;
//     string sTrackIR_exe_location;

//     CConfig::CConfig(int num_monitors, CDisplay[] displays);

// };

struct bounds_in_degrees {
    float  left;
    float  right;
    float  top;
    float  bottom;

    int pad_left;
    int pad_right;
    int pad_top;
    int pad_bottom;
};


class CConfig {
public:

    int profile_ID;
    bool bWatchdog;
    std::string sTrackIR_dll_location;

    bounds_in_degrees bounds[DEFAULT_MAX_DISPLAYS];

    CConfig() {};
    void LoadSettings(int);


};

#endif /* TRACKIRMOUSE_CONFIG_H */