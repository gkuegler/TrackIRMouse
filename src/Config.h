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
    int paddingLeft = 0;
    int paddingRight = 0;
    int paddingTop = 0;
    int paddingBottom = 0;
};

class CConfig {
public:

    // Values Stored in TOML File
    int m_profileID = 0;
    bool m_bWatchdog = 0;
    std::string m_sTrackIrDllLocation;
    int m_activeDisplayProfile = 0;

    // Values Determined at Run Time
    int m_monitorCount = 0;

    bounds_in_degrees bounds[DEFAULT_MAX_DISPLAYS];

    CConfig(){};

    void LoadSettings();
    void SaveSettings();

    void SetGeneralInteger(const char*, int);

private:
    toml::value m_vData;
    toml::value m_vDefaultPaddingTable;
    toml::value m_vProfilesTable;
};

#endif /* TRACKIRMOUSE_CONFIG_H */