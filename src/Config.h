#ifndef TRACKIRMOUSE_CONFIG_H
#define TRACKIRMOUSE_CONFIG_H

#include "Constants.h"

#define TOML11_PRESERVE_COMMENTS_BY_DEFAULT
#include "toml.hpp"

#include <string>

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

    template <typename T>
    int SetValueInTable(std::vector<std::string> tableHierarchy, std::string parameterName, T value)
    {
        toml::value* table = this->FindHighestTable(tableHierarchy);
        if (nullptr == table) return 1;

        // See link below for explanation on accessing the underlying
        // unordered_map of a toml table
        // https://github.com/ToruNiina/toml11/issues/85

        table->as_table()[parameterName] = value;
        return 0;
    }

private:
    //toml::value* FindParameterInTable(std::vector<std::string> tableHierarchy, std::string parameterName);
    toml::value* FindHighestTable(std::vector<std::string> tableHierarchy);
    toml::value m_vData;
    toml::value m_vDefaultPaddingTable;
    toml::value m_vProfilesTable;
};

#endif /* TRACKIRMOUSE_CONFIG_H */