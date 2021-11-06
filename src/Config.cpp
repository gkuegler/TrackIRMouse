#include "Config.h"

#include "Log.h"
#include "Exceptions.h"

#define FMT_HEADER_ONLY
#include <fmt\format.h>

#include <Windows.h>

#include <iostream>
#include <string>

CConfig g_config = CConfig();

typedef struct _RegistryQuery {
    int result;
    std::string resultString;
    std::string value;
} RegistryQuery;

RegistryQuery GetStringFrOomRegistry(HKEY hParentKey, const char* subKey, const char* subValue)
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

void CConfig::LoadSettings()
{
    m_monitorCount = GetSystemMetrics(SM_CMONITORS);
    
    // TOML will throw a std::runtime_error if there's a problem opening the file 
    m_vData = toml::parse<toml::preserve_comments>("settings.toml");

    // Find the general settings table
    auto& vGeneralSettings = toml::find(m_vData, "General");

    // find_or will return a default if parameter not found
    m_bWatchdog = toml::find_or<bool>(vGeneralSettings, "watchdog_enabled", 0);
    m_activeDisplayProfile = toml::find<int>(vGeneralSettings, "profile");

    //////////////////////////////////////////////////////////////////////
    //                  Finding NPTrackIR DLL Location                  //
    //////////////////////////////////////////////////////////////////////

    // Optionally the user can specify the location to the trackIR dll
    m_sTrackIrDllLocation = toml::find_or<std::string>(vGeneralSettings, "TrackIR_dll_directory", "default");

    if ("default" == m_sTrackIrDllLocation)
    {
        RegistryQuery path = GetStringFrOomRegistry(HKEY_CURRENT_USER, "Software\\NaturalPoint\\NATURALPOINT\\NPClient Location", "Path");

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

    // Using the 'find' method here instead of 'find_or' so that the user can be
    // notified if the default padding table hasn't been configured correctly
    // in the settings file.
    int defaultPaddingLeft = 0;
    int defaultPaddingRight = 0;
    int defaultPaddingTop = 0;
    int defaultPaddingBottom = 0;

    // Catch padding table errors and notify user, because reverting to e
    // 0 padding is not a critical to program function
    try {
        toml::value vDefaultPaddingTable = toml::find(m_vData, "DefaultPadding");

        defaultPaddingLeft = toml::find<int>(vDefaultPaddingTable, "left");
        defaultPaddingRight = toml::find<int>(vDefaultPaddingTable, "right");
        defaultPaddingTop = toml::find<int>(vDefaultPaddingTable, "top");
        defaultPaddingBottom = toml::find<int>(vDefaultPaddingTable, "bottom");

    }
    catch (std::out_of_range e) {
        LogToWixError(fmt::format("Exception with the default padding table."));
        LogToWixError(fmt::format("TOML Non Crititcal Exception Thrown.\n{}\n", e.what()));
    }

    //////////////////////////////////////////////////////////////////////
    //                       Find Display Mapping                       //
    //////////////////////////////////////////////////////////////////////

    LogToWix(fmt::format("\n{:-^50}\n", "User Mapping Info"));

    // Find the profiles table that contains all mapping profiles
    toml::value vProfilesTable = toml::find(m_vData, "Profiles");

    // Find the profile table which is currently enabled
    std::string profileTableName = std::to_string(m_activeDisplayProfile);
    auto& vActiveProfileTable = toml::find(vProfilesTable, profileTableName);

    // Load in current profile dependent settings
    m_profile_ID = toml::find_or<int>(vActiveProfileTable, "profile_ID", 13302);

    // Load in Display Mappings
    // Find the display mapping table for the given profile
    auto& vDisplayMappingTable = toml::find(vActiveProfileTable, "DisplayMappings");

    LogToWix(fmt::format("Padding\n"));

    // TODO: check bounds of my array first with number of monitors
    //  and see if they match
    
    for (int i = 0; i < m_monitorCount; i++) {

        std::string displayTableName = std::to_string(i);
        
        try {
            const auto& vDisplayMapping = toml::find(vDisplayMappingTable, displayTableName);

            // Bring In The Rotational Bounds
            toml::value left = toml::find(vDisplayMapping, "left");
            toml::value right = toml::find(vDisplayMapping, "right");
            toml::value top = toml::find(vDisplayMapping, "top");
            toml::value bottom = toml::find(vDisplayMapping, "bottom");

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
            int paddingLeft = toml::find_or<int>(vDisplayMapping, "paddingLeft", 5555);
            int paddingRight = toml::find_or<int>(vDisplayMapping, "paddingRight", 5555);
            int paddingTop = toml::find_or<int>(vDisplayMapping, "paddingTop", 5555);
            int paddingBottom = toml::find_or<int>(vDisplayMapping, "paddingBottom", 5555);

            if (paddingLeft != 5555) {
                LogToWix(fmt::format("Display {} Left:     {:>12}\n", i, paddingLeft));
                paddingLeft = paddingLeft;
            }
            else {
                LogToWix(fmt::format("Display {} Left:     {:>12} (Default)\n", i, defaultPaddingLeft));
                paddingLeft = defaultPaddingLeft;
            }
            if (paddingRight != 5555) {
                LogToWix(fmt::format("Display {} Right:    {:>12}\n", i, paddingRight));
                paddingRight = paddingRight;
            }
            else {
                LogToWix(fmt::format("Display {} Right:    {:>12} (Default)\n", i, defaultPaddingRight));
                paddingRight = defaultPaddingRight;
            }
            if (paddingTop != 5555) {
                LogToWix(fmt::format("Display {} Top:      {:>12}\n", i, paddingTop));
                paddingTop = paddingTop;
            }
            else {
                LogToWix(fmt::format("Display {} Top:      {:>12} (Default)\n", i, defaultPaddingTop));
                paddingTop = defaultPaddingTop;
            }
            if (paddingBottom != 5555) {
                LogToWix(fmt::format("Display {} Bottom:   {:>12}\n", i, paddingBottom));
                paddingBottom = paddingBottom;
            }
            else {
                LogToWix(fmt::format("Display {} Bottom:   {:>12} (Default)\n", i, defaultPaddingBottom));
                paddingBottom = defaultPaddingBottom;
            }

            m_bounds.push_back(bounds_in_degrees({rotLeft, rotRight, rotTop, rotBottom}, {paddingLeft, paddingRight, paddingTop, paddingBottom}));
        }
        catch (toml::type_error e)
        {
            LogToWixError(fmt::format("TOML Exception Thrown!\nIncorrect configuration of display:{}\n{}\n", i, e.what()));
        }
        catch (std::out_of_range e)
        {
            LogToWixError(fmt::format("TOML Exception Thrown!\nIncorrect configuration of display:{}\n{}\n", i, e.what()));
            // I wanted to throw std::runtime_error, but i haven't figured out how yet
            //throw 23;
        }
    }

}

void CConfig::SaveSettings()
{
    const std::string FileName = "settings_test.toml";

    //SetValue("General/track_on_start", usrTrackOnStart);
    //SetValue("General/quick_on_loss_of_track_ir", usrQuitOnLossOfTrackIr);
    //SetValue("General/watchdog_enabled", m_bWatchdog);
    //SetValue("General/profile", m_activeDisplayProfile);

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
    }
}

void CConfig::LogTomlError(const std::exception& ex)
{
    wxLogError("Incorrect type on reading configuration parameter: %s", ex.what());
}