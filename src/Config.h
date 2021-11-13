#ifndef TRACKIRMOUSE_CONFIG_H
#define TRACKIRMOUSE_CONFIG_H

#include "Constants.h"
#include "Log.h"

#define TOML11_PRESERVE_COMMENTS_BY_DEFAULT
#include "toml.hpp"

#include <string>

// struct bounds_in_degrees {
//     // Left, Right, Top, Bottom
//     std::array<std::string, 4> names = { "left", "right", "top", "bottom" };
//     std::array<float, 4> rotationBounds;
//     std::array<int, 4> paddingBounds;
// };

class bounds_in_degrees
{
public:

    // Left, Right, Top, Bottom
    static constexpr std::array<std::string_view, 4> names = { "left", "right", "top", "bottom" };
    std::array<float, 4> rotationBounds;
    std::array<int, 4> paddingBounds;

    bounds_in_degrees(std::array<float, 4>&& rotations, std::array<int, 4>&& padding)
    {
        rotationBounds = rotations;
        paddingBounds = padding;
    }
};

class CDisplayConfiguration
{
public:
    std::string m_name = "Example Text";
    int m_profile_ID = 0;
    bool m_useDefaultPadding = true;

    std::vector<bounds_in_degrees> m_bounds;
};

class CConfig
{
public:

    // Values Stored in TOML File
    // bool usrTrackOnStart = 0;
    // bool usrQuitOnLossOfTrackIr = 0;
    // bool m_bWatchdog = 0;

    std::string m_sTrackIrDllLocation = "";

    // int m_activeDisplayProfile = 0;
    int m_profile_ID = 0;

    // Values Determined at Run Time
    int m_monitorCount = 0;

    //bounds_in_degrees bounds[DEFAULT_MAX_DISPLAYS];
    // std::vector<bounds_in_degrees> m_bounds;
    CDisplayConfiguration m_activeDisplayConfiguration;

    // Using the 'find' method here instead of 'find_or' so that the user can be
    // notified if the default padding table hasn't been configured correctly
    // in the settings file.
    int m_defaultPaddingLeft = 0;
    int m_defaultPaddingRight = 0;
    int m_defaultPaddingTop = 0;
    int m_defaultPaddingBottom = 0;

    std::vector<std::string> m_profileNames;



    CConfig() {};

    void ParseFile(const std::string);
    void LoadSettings();
    void SaveSettings();
    void LoadActiveDisplay(std::string activeProfile);

    CDisplayConfiguration GetActiveDisplaySetUp()
    {
        return m_activeDisplayConfiguration;
    }
    


    template <typename T>
    int SetValueInTable(std::vector<std::string> tableHierarchy, std::string parameterName, const T value)
    {
        toml::value* table = this->FindHighestTable(tableHierarchy);
        if (nullptr == table) return 1;

        // See link below for explanation on accessing the underlying
        // unordered_map of a toml table
        // https://github.com/ToruNiina/toml11/issues/85

        table->as_table()[parameterName] = value;

        return 0;
    }

    template <typename T>
    int SetValue(std::string s, const T value)
    {
        try {
            std::vector<std::string> tableHierarchy;

            constexpr std::string_view delimiter = "/";
            size_t last = 0;
            size_t next = 0;

            while ((next = s.find(delimiter, last)) != std::string::npos)
            {
                std::string key = s.substr(last, next - last);
                last = next + 1;
                tableHierarchy.push_back(key);
            }
            std::string parameterName = s.substr(last);

            return SetValueInTable(tableHierarchy, parameterName, value);
        }
        catch (const std::exception& ex)
        {
            LogToWixError(fmt::format("A big exception happened: {}\n", ex.what()));
        }
    }

    
    int GetInteger(std::string s);
    float GetFloat(std::string s);
    bool GetBool(std::string s);
    std::string GetString(std::string s);

private:
    toml::value m_vData;

    void LogTomlError(const std::exception& ex);
    toml::value GetValue(std::string s);
    toml::value* FindHighestTable(std::vector<std::string> tableHierarchy);
};

extern CConfig g_config;

#endif /* TRACKIRMOUSE_CONFIG_H */