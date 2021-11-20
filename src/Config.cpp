#include "Config.h"

#include "Log.h"
#include "Exceptions.h"

#include <Windows.h>

#include <iostream>
#include <string>

CConfig g_config = CConfig();

typedef struct _RegistryQuery {
    int result;
    std::string resultString;
    std::string value;
} RegistryQuery;

RegistryQuery GetStringFromRegistry(HKEY hParentKey, const char* subKey, const char* subValue)
{

    //////////////////////////////////////////////////////////////////////
    //                         Opening The Key                          //
    //////////////////////////////////////////////////////////////////////

    HKEY hKey = 0;

    LSTATUS statusOpen = RegOpenKeyExA(
        hParentKey, // should usually be HKEY_CURRENT_USER
        subKey,
        0, //[in]           DWORD  ulOptions,
        KEY_READ, //[in]           REGSAM samDesired,
        &hKey
    );

    if (ERROR_FILE_NOT_FOUND == statusOpen)
    {
        return RegistryQuery{ ERROR_FILE_NOT_FOUND , "Registry key not found.", "" };
    }

    // Catch all other errors
    if (ERROR_SUCCESS != statusOpen)
    {
        return RegistryQuery{ statusOpen , "Could not open registry key.", "" };
    }


    //////////////////////////////////////////////////////////////////////
    //                    Querying Value Information                    //
    //////////////////////////////////////////////////////////////////////

    DWORD valueType = 0;
    DWORD sizeOfBuffer = 0;

    LSTATUS statusQueryValue = RegQueryValueExA(
        hKey, // [in]                HKEY    hKey,
        "Path",// [in, optional]      LPCSTR  lpValueName,
        0,// LPDWORD lpReserved,
        &valueType, // [out, optional]     LPDWORD lpType,
        0, // [out, optional]     LPBYTE  lpData,
        &sizeOfBuffer // [in, out, optional] LPDWORD lpcbData
    );

    if (ERROR_FILE_NOT_FOUND == statusQueryValue)
    {
        return RegistryQuery{ ERROR_FILE_NOT_FOUND, "Value not found for key.", "" };
    }

    // Catch all other errors of RegQueryValueExA
    if (ERROR_SUCCESS != statusQueryValue)
    {
        return RegistryQuery{ statusQueryValue, "RegQueryValueExA failed.", "" };
    }

    if (REG_SZ != valueType)
    {
        return RegistryQuery{ 1, "Registry value not a string type.", "" };
    }


    //////////////////////////////////////////////////////////////////////
    //                      Getting the hKey Value                       //
    //////////////////////////////////////////////////////////////////////

    // Registry key may or may not be stored with a null terminator
    // add one just in case
    char* szPath = static_cast<char*>(calloc(1, sizeOfBuffer + 1));

    if (NULL == szPath)
    {
        return RegistryQuery{ 1, "Failed to allocate memory.", "" };
    }

    LSTATUS statusGetValue = RegGetValueA(
        hKey, // [in]                HKEY    hkey,
        0, // [in, optional]      LPCSTR  lpSubKey,
        subValue, // [in, optional]      LPCSTR  lpValue,
        RRF_RT_REG_SZ, // [in, optional]      DWORD   dwFlags,
        &valueType, // [out, optional]     LPDWORD pdwType,
        (void*)szPath, // [out, optional]     PVOID   pvData,
        &sizeOfBuffer // [in, out, optional] LPDWORD pcbData
    );

    return RegistryQuery{ 0, "", std::string(szPath) };
}

void CConfig::ParseFile(const std::string fileName)
{
    m_vData = toml::parse<toml::preserve_comments>(fileName);
}

// Should be changed to verify
void CConfig::LoadSettings()
{
    m_monitorCount = GetSystemMetrics(SM_CMONITORS);

    // Find the general settings table
    auto& vGeneralSettings = toml::find(m_vData, "General");

    //////////////////////////////////////////////////////////////////////
    //                   Validate In General Settings                   //
    //////////////////////////////////////////////////////////////////////

    // Values Stored in TOML File
    bool usrTrackOnStart = toml::find<bool>(vGeneralSettings, "track_on_start");
    bool usrQuitOnLossOfTrackIr = toml::find<bool>(vGeneralSettings, "quit_on_loss_of_track_ir");
    bool bWatchdog = toml::find<bool>(vGeneralSettings, "watchdog_enabled");

    std::string activeDisplayProfile = toml::find<std::string>(vGeneralSettings, "active_profile");

    //////////////////////////////////////////////////////////////////////
    //                  Finding NPTrackIR DLL Location                  //
    //////////////////////////////////////////////////////////////////////

    // Optionally the user can specify the location to the trackIR dll
    m_sTrackIrDllLocation = toml::find<std::string>(vGeneralSettings, "trackir_dll_directory");

    if ("default" == m_sTrackIrDllLocation)
    {
        RegistryQuery path = GetStringFromRegistry(HKEY_CURRENT_USER, "Software\\NaturalPoint\\NATURALPOINT\\NPClient Location", "Path");

        if (0 == path.result)
        {
            m_sTrackIrDllLocation = path.value;
            LogToWix("Acquired DLL location from registry.\n");
        }
        else
        {
            LogToWixError(fmt::format("Registry get key failed."));
            LogToWixError(fmt::format("  result: {}", path.result));
            LogToWixError(fmt::format("  result string: {}", path.resultString));
            throw Exception("See error above.");
        }

        
    }

    LogToWix(fmt::format("NPTrackIR DLL Location: {}", m_sTrackIrDllLocation));

    // Check if DLL folder path is post fixed with slashes
    if (m_sTrackIrDllLocation.back() != '\\')
    {
        m_sTrackIrDllLocation.push_back('\\');
    }

    // Match to the correct bitness of this application
    #if defined(_WIN64) || defined(__amd64__)
        m_sTrackIrDllLocation.append("NPClient64.dll");
    #else	    
        m_sTrackIrDllLocation.append("NPClient.dll");
    #endif


    //////////////////////////////////////////////////////////////////////
    //                     Finding Default Padding                      //
    //////////////////////////////////////////////////////////////////////

    // Catch padding table errors and notify user, because reverting to e
    // 0 padding is not a critical to program function
    toml::value vDefaultPaddingTable;

    try
    {
        vDefaultPaddingTable = toml::find(m_vData, "DefaultPadding");
    }
    catch (std::out_of_range e)
    {
        LogToWixError(fmt::format("Default Padding Table Not Found"));
        LogToWixError(fmt::format(
            "Please add the following to the settings.toml file:\n"
            "[DefaultPadding]"
            "left   = 0"
            "right  = 0"
            "top    = 0"
            "bottom = 0"
        ));
    }

    m_defaultPaddingLeft = toml::find<int>(vDefaultPaddingTable, "left");
    m_defaultPaddingRight = toml::find<int>(vDefaultPaddingTable, "right");
    m_defaultPaddingTop = toml::find<int>(vDefaultPaddingTable, "top");
    m_defaultPaddingBottom = toml::find<int>(vDefaultPaddingTable, "bottom");

    //////////////////////////////////////////////////////////////////////
    //                       Find Display Mapping                       //
    //////////////////////////////////////////////////////////////////////

    LogToWix(fmt::format("\n{:-^50}\n", "User Mapping Info"));


    auto& vProfilesArray = toml::find(m_vData, "Profiles");

    for (auto& table: vProfilesArray.as_array())
    {
        try
        {
            std::string profileName = toml::find<std::string>(table, "name");
            m_profileNames.push_back(profileName);
        }
        catch (std::out_of_range e)
        {
            LogToWixError(fmt::format("TOML Exception Thrown!\nIncorrect configuration of display.\n{}\n", e.what()));
        }    
    }
    
    LoadActiveDisplay(activeDisplayProfile);

}

void CConfig::LoadActiveDisplay(std::string activeProfile)
{
    CDisplayConfiguration configuration;
    // Find the profiles table that contains all mapping profiles.
    auto& vProfilesArray = toml::find(m_vData, "Profiles");
    std::string tableKey;
    toml::value vActiveProfileTable;

    // Find the table with a matching profile name.
    // .as_table() returns a std::unordered_map<toml::key, toml::table>
    // Conversion is necessary to loop by element.
    for (auto& table: vProfilesArray.as_array())
    {
        std::string profileName = toml::find<std::string>(table, "name");
        if (activeProfile == profileName)
        {
            vActiveProfileTable = table;
            break;
        }
    }

    // Use the found table key
    // auto& vActiveProfileTable = toml::find(vProfilesArray, tableKey);

    // Load in current profile dependent settings
    configuration.m_profile_ID = toml::find_or<int>(vActiveProfileTable, "profile_id", 13302);
    configuration.m_name = activeProfile;

    // Find the display mapping table for the given profile
    auto& vDisplayMappingArray = toml::find(vActiveProfileTable, "DisplayMappings");
    
    int index = 0;
    for (auto& display: vDisplayMappingArray.as_array())
    {
        //std::string i = display.first;
        //auto& vDisplayMapping = display.second;
        int i = index++;

        try
        {
            // Bring In The Rotational Bounds
            toml::value left = toml::find(display, "left");
            toml::value right = toml::find(display, "right");
            toml::value top = toml::find(display, "top");
            toml::value bottom = toml::find(display, "bottom");

            // Each value is checked because this toml library cannot
            // convert integers in a toml value to a float
            toml::value_t integer = toml::value_t::integer;

            float rotLeft = (left.type() == integer) ? static_cast<float>(left.as_integer())
                : left.as_floating();

            float rotRight = (right.type() == integer) ? static_cast<float>(right.as_integer())
                : right.as_floating();

            float rotTop = (top.type() == integer) ? static_cast<float>(top.as_integer())
                : top.as_floating();

            float rotBottom = (bottom.type() == integer) ? static_cast<float>(bottom.as_integer())
                : bottom.as_floating();

            // I return an ungodly fake high padding numbelong ,
            // so that I can tell if one was found in the toml config file
            // without producing an exception if a value was not found.
            // Padding values are not critical the program operation.
            int paddingLeft = toml::find_or<int>(display, "paddingLeft", 5555);
            int paddingRight = toml::find_or<int>(display, "paddingRight", 5555);
            int paddingTop = toml::find_or<int>(display, "paddingTop", 5555);
            int paddingBottom = toml::find_or<int>(display, "paddingBottom", 5555);

            if (paddingLeft != 5555)
            {
                LogToWix(fmt::format("Display {} Left:     {:>12}\n", i, paddingLeft));
                paddingLeft = paddingLeft;
            }
            else
            {
                LogToWix(fmt::format("Display {} Left:     {:>12} (Default)\n", i, m_defaultPaddingLeft));
                paddingLeft = m_defaultPaddingLeft;
            }
            if (paddingRight != 5555)
            {
                LogToWix(fmt::format("Display {} Right:    {:>12}\n", i, paddingRight));
                paddingRight = paddingRight;
            }
            else
            {
                LogToWix(fmt::format("Display {} Right:    {:>12} (Default)\n", i, m_defaultPaddingRight));
                paddingRight = m_defaultPaddingRight;
            }
            if (paddingTop != 5555)
            {
                LogToWix(fmt::format("Display {} Top:      {:>12}\n", i, paddingTop));
                paddingTop = paddingTop;
            }
            else
            {
                LogToWix(fmt::format("Display {} Top:      {:>12} (Default)\n", i, m_defaultPaddingTop));
                paddingTop = m_defaultPaddingTop;
            }
            if (paddingBottom != 5555)
            {
                LogToWix(fmt::format("Display {} Bottom:   {:>12}\n", i, paddingBottom));
                paddingBottom = paddingBottom;
            }
            else
            {
                LogToWix(fmt::format("Display {} Bottom:   {:>12} (Default)\n", i, m_defaultPaddingBottom));
                paddingBottom = m_defaultPaddingBottom;
            }

            configuration.m_bounds.push_back(bounds_in_degrees({ rotLeft, rotRight, rotTop, rotBottom }, { paddingLeft, paddingRight, paddingTop, paddingBottom }));
        }
        catch (toml::type_error e)
        {
            LogToWixError(fmt::format("TOML Exception Thrown!\nIncorrect configuration of display:{}\n{}\n", i, e.what()));
        }
        catch (std::out_of_range e)
        {
            LogToWixError(fmt::format("TOML Exception Thrown!\nIncorrect configuration of display:{}\n{}\n", i, e.what()));
        }
    }

    SetValue("General/active_profile", activeProfile);
    m_activeDisplayConfiguration = configuration;

}

void CConfig::AddDisplayConfiguration()
{
    CDisplayConfiguration configuration;

    // configuration.m_name = "";
    // configuration.m_profile_ID = 0;
    // configuration.m_useDefaultPadding = true;
    // configuration.m_bounds;
}

void CConfig::SaveSettings()
{
    const std::string FileName = "settings.toml";

    std::fstream file(FileName, std::ios_base::out);
    file << m_vData << std::endl;
    file.close();
}

toml::value* CConfig::FindHighestTable(std::vector<std::string> tableHierarchy)
{
    toml::value* table = &m_vData;

    if (tableHierarchy.empty())
    {
        return table;
    }

    for (auto& tableName : tableHierarchy)
    {
        if (tableName.empty())
        {
            LogToWixError("Tablename can't be empty.");
            return nullptr;
        }

        table = &toml::get<toml::table>(*table).at(tableName);
    }

    return table;
}

toml::value CConfig::GetValue(std::string s)
{
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

    toml::value* table = this->FindHighestTable(tableHierarchy);
    if (nullptr == table) return 1;

    return toml::find(*table, parameterName);

    return 0;
}

int CConfig::GetInteger(std::string s)
{

    try
    {
        toml::value value = GetValue(s);
        return value.as_integer();
    }
    catch (const std::exception& ex)
    {
        LogTomlError(ex);
        return -1;
    }
}

float CConfig::GetFloat(std::string s)
{
    try
    {
        toml::value value = GetValue(s);
        return value.as_floating();
    }
    catch (const std::exception& ex)
    {
        LogTomlError(ex);
        return -1.0;
    }
}

bool CConfig::GetBool(std::string s)
{
    try
    {
        toml::value value = GetValue(s);
        return value.as_boolean();
    }
    catch (const std::exception& ex)
    {
        LogTomlError(ex);
        return false;
    }
}

std::string CConfig::GetString(std::string s)
{
    try
    {
        toml::value value = GetValue(s);
        return value.as_string();
    }
    catch (const std::exception& ex)
    {
        LogTomlError(ex);
        return "";
    }
}

void CConfig::LogTomlError(const std::exception& ex)
{
    wxLogFatalError("Incorrect type on reading configuration parameter: %s", ex.what());
}


CConfig* GetGlobalConfig()
{
    return &g_config;
}

CConfig GetGlobalConfigCopy()
{
    return g_config;
}