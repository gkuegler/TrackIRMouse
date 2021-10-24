#include "Config.h"
#include "Log.h"
#include "Exceptions.h"

#define FMT_HEADER_ONLY
#include <fmt\format.h>
#include "toml.hpp"

#include <Windows.h>

#include <iostream>
#include <string>

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

    int defaultPaddingLeft = 0;
    int defaultPaddingRight = 0;
    int defaultPaddingTop = 0;
    int defaultPaddingBottom = 0;
    
    std::string d = "display";

    // TOML will throw a std::runtime_error if there's a problem opening the file 
    auto vData = toml::parse("settings.toml");

    // Find the general settings table
    auto& vGeneralSettings = toml::find(vData, "general");

    // find_or will return a default if parameter not found
    m_bWatchdog = toml::find_or<bool>(vGeneralSettings, "watchdog_enabled", 0);
    m_displayProfile = toml::find<int>(vGeneralSettings, "profile");

    //Optionally the user can specify the location to the trackIR dll
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
            LogToWix(fmt::format("Registry get key failed."));
            LogToWix(fmt::format("  result: {}", path.result));
            LogToWix(fmt::format("  result string: {}", path.resultString));
            throw Exception("See error above.");
        }

        
    }
    LogToWix(fmt::format("NPTrackIR DLL Location: {}", m_sTrackIrDllLocation));

    if (m_sTrackIrDllLocation.back() != '\\')
    {
        m_sTrackIrDllLocation.push_back('\\');
    }

    #if defined(_WIN64) || defined(__amd64__)
        m_sTrackIrDllLocation.append("NPClient64.dll");
    #else	    
        m_sTrackIrDllLocation.append("NPClient.dll");
    #endif

    // load in the global default padding table if available
    try {
        m_vDefaultPaddings = toml::find(vData, "default_padding");
        defaultPaddingLeft = toml::find<int>(m_vDefaultPaddings, "left");
        defaultPaddingRight = toml::find<int>(m_vDefaultPaddings, "right");
        defaultPaddingTop = toml::find<int>(m_vDefaultPaddings, "top");
        defaultPaddingBottom = toml::find<int>(m_vDefaultPaddings, "bottom");

    }
    catch (std::out_of_range e) {
        LogToWix(fmt::format("Exception with the default padding table."));
        LogToWix(fmt::format("TOML Non Crititcal Exception Thrown.\n{}\n", e.what()));
    }

    LogToWix(fmt::format("\n{:-^50}\n", "User Mapping Info"));

    // Find the profiles table that contains all profiles
    m_vDisplayMappingProfiles = toml::find(vData, "profiles");

    // Find the profile table which is currently enabled enabled
    std::string profileTableName = std::to_string(m_displayProfile);
    auto& vProfileData = toml::find(m_vDisplayMappingProfiles, profileTableName);

    // Load in current profile dependent settings
    m_profileID = toml::find_or<int>(vProfileData, "profile_ID", 13302);

    // Load in Display Mappings
    // Find the display mapping table for the given profile
    auto& vDisplayMapping = toml::find(vProfileData, "display");

    LogToWix(fmt::format("Padding\n"));

    for (int i = 0; i < m_monitorCount; i++) {
        std::string tname = std::to_string(i);
        //assert(i < m_monitorCount);
        try {
            const auto& vTomlDisplay = toml::find(vDisplayMapping, tname);

            // Bring In The Rotational Bounds
            toml::value left = toml::find(vTomlDisplay, "left");
            toml::value right = toml::find(vTomlDisplay, "right");
            toml::value top = toml::find(vTomlDisplay, "top");
            toml::value bottom = toml::find(vTomlDisplay, "bottom");

            // Each value is checked because this toml library cannot
            // cast integers in a toml file to a float
            if (left.type() == toml::value_t::integer)
            {
                bounds[i].left = static_cast<float>(left.as_integer());
            }
            else
            {
                bounds[i].left = toml::find<float>(vTomlDisplay, "left"); 
            }
            if (right.type() == toml::value_t::integer)
            {
                bounds[i].right = static_cast<float>(right.as_integer());
            }
            else
            {
                bounds[i].right = toml::find<float>(vTomlDisplay, "right");
            }
            if (top.type() == toml::value_t::integer)
            {
                bounds[i].top = static_cast<float>(top.as_integer());
            }
            else
            {
                bounds[i].top = toml::find<float>(vTomlDisplay, "top");
            }
            if (bottom.type() == toml::value_t::integer)
            {
                bounds[i].bottom = static_cast<float>(bottom.as_integer());
            }
            else
            {
                bounds[i].bottom = toml::find<float>(vTomlDisplay, "bottom");
            }

            // I return an ungodly fake high padding number,
            // so that I can tell of one was found in the toml config file
            // without producing an exception if a value was not found.
            // Padding values are not critical the program operation.
            int paddingLeft = toml::find_or<int>(vTomlDisplay, "paddingLeft", 5555);
            int paddingRight = toml::find_or<int>(vTomlDisplay, "paddingRight", 5555);
            int paddingTop = toml::find_or<int>(vTomlDisplay, "paddingTop", 5555);
            int paddingBottom = toml::find_or<int>(vTomlDisplay, "paddingBottom", 5555);

            if (paddingLeft != 5555) {
                LogToWix(fmt::format("Display {} Left:     {:>12}\n", i, paddingLeft));
                bounds[i].paddingLeft = paddingLeft;
            }
            else {
                LogToWix(fmt::format("Display {} Left:     {:>12} (Default)\n", i, defaultPaddingLeft));
                bounds[i].paddingLeft = defaultPaddingLeft;
            }
            if (paddingRight != 5555) {
                LogToWix(fmt::format("Display {} Right:    {:>12}\n", i, paddingRight));
                bounds[i].paddingRight = paddingRight;
            }
            else {
                LogToWix(fmt::format("Display {} Right:    {:>12} (Default)\n", i, defaultPaddingRight));
                bounds[i].paddingRight = defaultPaddingRight;
            }
            if (paddingTop != 5555) {
                LogToWix(fmt::format("Display {} Top:      {:>12}\n", i, paddingTop));
                bounds[i].paddingTop = paddingTop;
            }
            else {
                LogToWix(fmt::format("Display {} Top:      {:>12} (Default)\n", i, defaultPaddingTop));
                bounds[i].paddingTop = defaultPaddingTop;
            }
            if (paddingBottom != 5555) {
                LogToWix(fmt::format("Display {} Bottom:   {:>12}\n", i, paddingBottom));
                bounds[i].paddingBottom = paddingBottom;
            }
            else {
                LogToWix(fmt::format("Display {} Bottom:   {:>12} (Default)\n", i, defaultPaddingBottom));
                bounds[i].paddingBottom = defaultPaddingBottom;
            }
        }
        catch (toml::type_error e)
        {
            LogToWix(fmt::format("TOML Exception Thrown!\nIncorrect configuration of display:{}\n{}\n", i, e.what()));
        }
        catch (std::out_of_range e)
        {
            LogToWix(fmt::format("TOML Exception Thrown!\nIncorrect configuration of display:{}\n{}\n", i, e.what()));
            // I wanted to throw std::runtime_error, but i haven't figured out how yet
            //throw 23;
        }
    }

}


void CConfig::SaveSettings()
{
    toml::value vGeneralSettings{ {"watchdog_enabled", m_bWatchdog}, {"TrackIR_dll_directory", m_sTrackIrDllLocation}, {"profile", m_displayProfile}};

    //wxFile file;
    const std::string FileName = "settings_test.toml";

    //if (file.Exists(FileName))
    //{
    //    if (!file.Access(FileName, wxFile::write))
    //    {
    //        wxLogFatalError("Unable to open log file.");
    //    }
    //}

    //file.Open(FileName, wxFile::write);

    //bool success = file.Write(vGeneralSettings)

    //wxFileOutputStream file(FileName);

    //file << vGeneralSettings << std::endl;

    std::fstream file(FileName, std::ios_base::out);
    //file.open();
    file << vGeneralSettings << std::endl;
    file.close();

}