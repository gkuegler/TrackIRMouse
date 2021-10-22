#ifndef TRACKIRMOUSE_CONFIG_H
#define TRACKIRMOUSE_CONFIG_H

#include <string>
#include "Constants.h"
#include "toml.hpp"

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

    // Values Stored in TOML File
    int m_profile_ID = 0;
    bool m_bWatchdog = 0;
    std::string m_sTrackIR_dll_location;
    int m_display_profile = 0;

    // Values Determined at Run Time
    int m_iMonitorCount = 0;

    bounds_in_degrees bounds[DEFAULT_MAX_DISPLAYS];

    CConfig() {};
    void LoadSettings();
    void SaveSettings();

private:
    toml::value m_default_padding_table;
    toml::value m_display_mapping_profiles;
};

#endif /* TRACKIRMOUSE_CONFIG_H */