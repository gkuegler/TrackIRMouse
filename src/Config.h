#ifndef TRACKIRMOUSE_CONFIG_H
#define TRACKIRMOUSE_CONFIG_H

#include "Constants.h"
#include "Log.h"

#define TOML11_PRESERVE_COMMENTS_BY_DEFAULT
#include "toml.hpp"

#include <string>


class CBounds
{
public:

    // Left, Right, Top, Bottom
    static constexpr std::array<std::string_view, 4> names = { "left", "right", "top", "bottom" };
    std::array<float, 4> rotationBounds{0.0, 0.0, 0.0, 0.0};
    std::array<int, 4> paddingBounds{0, 0, 0, 0};

    CBounds(std::array<float, 4>&& rotations, std::array<int, 4>&& padding)
    {
        rotationBounds = rotations;
        paddingBounds = padding;
    }
};

class CProfile
{
public:
    std::string m_name = "Lorem Ipsum";
    int m_profile_ID = 0;
    bool m_useDefaultPadding = true;

    std::vector<CBounds> m_bounds;
};

class CConfig
{
public:

    std::string m_sTrackIrDllLocation = "";
    int m_monitorCount = 0;

    int m_defaultPaddingLeft = 0;
    int m_defaultPaddingRight = 0;
    int m_defaultPaddingTop = 0;
    int m_defaultPaddingBottom = 0;

    CProfile m_activeProfile;

    CConfig() {};

    // Initializations Functions
    void ParseFile(const std::string);
    void LoadSettings();
    void SaveSettings();

    // Loads active profile from toml object into
    // m_activeProfile data structure
    void LoadActiveDisplay(std::string activeProfile);

    // Getter functions
    CProfile GetActiveProfile();
    std::vector <std::string> GetProfileNames();

    void AddProfile(std::string newProfileName);
    void RemoveProfile(std::string profileName);
    
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
            return -1;
        }
    }
   
    int GetInteger(std::string s);
    float GetFloat(std::string s);
    bool GetBool(std::string s);
    std::string GetString(std::string s);

private:
    toml::value m_vData; // holds main toml object

    void LogTomlError(const std::exception& ex);
    toml::value GetValue(std::string s);
    toml::value* FindHighestTable(std::vector<std::string> tableHierarchy);

    template <typename T>
    int SetValueInTable(std::vector<std::string> tableHierarchy, std::string parameterName, const T value)
    {
        toml::value* table = this->FindHighestTable(tableHierarchy);
        if (nullptr == table) return -1;

        // See link below for explanation on accessing the underlying
        // unordered_map of a toml table
        // https://github.com/ToruNiina/toml11/issues/85

        table->as_table()[parameterName] = value;

        return 0;
    }
};

CConfig* GetGlobalConfig();
CConfig GetGlobalConfigCopy();

extern CConfig g_config;

#endif /* TRACKIRMOUSE_CONFIG_H */