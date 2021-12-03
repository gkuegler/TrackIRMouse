#include "Config.h"

#include <Windows.h>

#include <iostream>
#include <string>

#include "Exceptions.h"
#include "Log.h"

CConfig g_config = CConfig();

CConfig *GetGlobalConfig() { return &g_config; }

CConfig GetGlobalConfigCopy() { return g_config; }

typedef struct _RegistryQuery {
  int result;
  std::string resultString;
  std::string value;
} RegistryQuery;

RegistryQuery GetStringFromRegistry(HKEY hParentKey, const char *subKey,
                                    const char *subValue) {
  //////////////////////////////////////////////////////////////////////
  //                         Opening The Key                          //
  //////////////////////////////////////////////////////////////////////

  HKEY hKey = 0;

  LSTATUS statusOpen =
      RegOpenKeyExA(hParentKey,  // should usually be HKEY_CURRENT_USER
                    subKey,
                    0,         //[in]           DWORD  ulOptions,
                    KEY_READ,  //[in]           REGSAM samDesired,
                    &hKey);

  if (ERROR_FILE_NOT_FOUND == statusOpen) {
    return RegistryQuery{ERROR_FILE_NOT_FOUND, "Registry key not found.", ""};
  }

  // Catch all other errors
  if (ERROR_SUCCESS != statusOpen) {
    return RegistryQuery{statusOpen, "Could not open registry key.", ""};
  }

  //////////////////////////////////////////////////////////////////////
  //                    Querying Value Information                    //
  //////////////////////////////////////////////////////////////////////

  DWORD valueType = 0;
  DWORD sizeOfBuffer = 0;

  LSTATUS statusQueryValue =
      RegQueryValueExA(hKey,        // [in]                HKEY    hKey,
                       "Path",      // [in, optional]      LPCSTR  lpValueName,
                       0,           // LPDWORD lpReserved,
                       &valueType,  // [out, optional]     LPDWORD lpType,
                       0,           // [out, optional]     LPBYTE  lpData,
                       &sizeOfBuffer  // [in, out, optional] LPDWORD lpcbData
      );

  if (ERROR_FILE_NOT_FOUND == statusQueryValue) {
    return RegistryQuery{ERROR_FILE_NOT_FOUND, "Value not found for key.", ""};
  }

  // Catch all other errors of RegQueryValueExA
  if (ERROR_SUCCESS != statusQueryValue) {
    return RegistryQuery{statusQueryValue, "RegQueryValueExA failed.", ""};
  }

  if (REG_SZ != valueType) {
    return RegistryQuery{1, "Registry value not a string type.", ""};
  }

  //////////////////////////////////////////////////////////////////////
  //                      Getting the hKey Value                       //
  //////////////////////////////////////////////////////////////////////

  // Registry key may or may not be stored with a null terminator
  // add one just in case
  char *szPath = static_cast<char *>(calloc(1, sizeOfBuffer + 1));

  if (NULL == szPath) {
    return RegistryQuery{1, "Failed to allocate memory.", ""};
  }

  LSTATUS statusGetValue =
      RegGetValueA(hKey,            // [in]                HKEY    hkey,
                   0,               // [in, optional]      LPCSTR  lpSubKey,
                   subValue,        // [in, optional]      LPCSTR  lpValue,
                   RRF_RT_REG_SZ,   // [in, optional]      DWORD   dwFlags,
                   &valueType,      // [out, optional]     LPDWORD pdwType,
                   (void *)szPath,  // [out, optional]     PVOID   pvData,
                   &sizeOfBuffer    // [in, out, optional] LPDWORD pcbData
      );

  if (ERROR_SUCCESS == statusOpen) {
    return RegistryQuery{0, "", std::string(szPath)};
  } else {
    return RegistryQuery{statusGetValue, "Could not get registry key.", ""};
  }
}

void CConfig::ParseFile(const std::string fileName) {
  m_vData = toml::parse<toml::preserve_comments>(fileName);
}

// Should be changed to verify
void CConfig::LoadSettings() {
  m_monitorCount = GetSystemMetrics(SM_CMONITORS);

  // Find the general settings table
  auto &vGeneralSettings = toml::find(m_vData, "General");

  // Verify values exist and parse to correct type
  data.trackOnStart = toml::find<bool>(vGeneralSettings, "track_on_start");
  data.quitOnLossOfTrackIr =
      toml::find<bool>(vGeneralSettings, "quit_on_loss_of_track_ir");
  data.watchdogEnabled = toml::find<bool>(vGeneralSettings, "watchdog_enabled");

  // Optionally the user can specify the location to the trackIR dll
  data.trackIrDllFolder =
      toml::find<std::string>(vGeneralSettings, "trackir_dll_directory");

  data.activeProfileName =
      toml::find<std::string>(vGeneralSettings, "active_profile");

  //////////////////////////////////////////////////////////////////////
  //                  Finding NPTrackIR DLL Location                  //
  //////////////////////////////////////////////////////////////////////

  if ("default" == data.trackIrDllFolder) {
    RegistryQuery path = GetStringFromRegistry(
        HKEY_CURRENT_USER,
        "Software\\NaturalPoint\\NATURALPOINT\\NPClient Location", "Path");

    if (0 == path.result) {
      m_trackIrDllPath = path.value;
      LogToFile("Acquired DLL location from registry.");
    } else {
      throw Exception(
          fmt::format("See error above.\n  result: {}"
                      "  result string: {}",
                      path.result, path.resultString));
    }
  } else {
    m_trackIrDllPath = data.trackIrDllFolder;
  }

  // Check if DLL folder path is postfixed with slashes
  if (m_trackIrDllPath.back() != '\\') {
    m_trackIrDllPath.push_back('\\');
  }

// Match to the correct bitness of this application
#if defined(_WIN64) || defined(__amd64__)
  m_trackIrDllPath.append("NPClient64.dll");
#else
  m_sTrackIrDllLocation.append("NPClient.dll");
#endif

  LogToFile(fmt::format("NPTrackIR DLL Path: {}", m_trackIrDllPath));

  //////////////////////////////////////////////////////////////////////
  //                     Finding Default Padding                      //
  //////////////////////////////////////////////////////////////////////

  // Catch padding table errors and notify user, because reverting to e
  // 0 padding is not a critical to program function
  toml::value vDefaultPaddingTable;

  try {
    vDefaultPaddingTable = toml::find(m_vData, "DefaultPadding");
  } catch (std::out_of_range e) {
    LogToWixError(fmt::format("Default Padding Table Not Found"));
    LogToWixError(
        fmt::format("Please add the following to the settings.toml file:\n"
                    "[DefaultPadding]"
                    "left   = 0"
                    "right  = 0"
                    "top    = 0"
                    "bottom = 0"));
  }

  data.defaultPaddings[0] = toml::find<int>(vDefaultPaddingTable, "left");
  data.defaultPaddings[1] = toml::find<int>(vDefaultPaddingTable, "right");
  data.defaultPaddings[2] = toml::find<int>(vDefaultPaddingTable, "top");
  data.defaultPaddings[3] = toml::find<int>(vDefaultPaddingTable, "bottom");

  //////////////////////////////////////////////////////////////////////
  //                      Find Profiles Mapping                       //
  //////////////////////////////////////////////////////////////////////

  // Find the profiles table that contains all mapping profiles.
  auto vProfilesArray = toml::find(m_vData, "Profiles");

  // Find the table with a matching profile name.
  // .as_table() returns a std::unordered_map<toml::key, toml::table>
  // Conversion is necessary to loop by element.
  // TODO: check with toml website to try and get better error messages when
  // validating parameters.6
  int i = 0;
  for (auto profile : vProfilesArray.as_array()) {
    // Load in current profile dependent settings

    SProfile newProfile;

    newProfile.name = toml::find<std::string>(profile, "name");
    newProfile.profile_ID = toml::find<int>(profile, "profile_id");
    newProfile.useDefaultPadding =
        toml::find<bool>(profile, "use_default_padding");

    // Find the display mapping table for the given profile
    auto &vDisplayMapping = toml::find(profile, "DisplayMappings");

    for (auto &display : vDisplayMapping.as_array()) {
      // Bring In The Rotational Bounds
      toml::value left = toml::find(display, "left");
      toml::value right = toml::find(display, "right");
      toml::value top = toml::find(display, "top");
      toml::value bottom = toml::find(display, "bottom");

      // Each value is checked because this toml library cannot
      // convert integers to floats from a toml value
      toml::value_t integer = toml::value_t::integer;

      float rotLeft = (left.type() == integer)
                          ? static_cast<float>(left.as_integer())
                          : left.as_floating();

      float rotRight = (right.type() == integer)
                           ? static_cast<float>(right.as_integer())
                           : right.as_floating();

      float rotTop = (top.type() == integer)
                         ? static_cast<float>(top.as_integer())
                         : top.as_floating();

      float rotBottom = (bottom.type() == integer)
                            ? static_cast<float>(bottom.as_integer())
                            : bottom.as_floating();

      // I return an ungodly fake high padding numbelong ,
      // so that I can tell if one was found in the toml config file
      // without producing an exception if a value was not found.
      // Padding values are not critical the program operation.
      int paddingLeft = toml::find_or<int>(display, "paddingLeft", 5555);
      int paddingRight = toml::find_or<int>(display, "paddingRight", 5555);
      int paddingTop = toml::find_or<int>(display, "paddingTop", 5555);
      int paddingBottom = toml::find_or<int>(display, "paddingBottom", 5555);

      if (paddingLeft == 5555) paddingLeft = data.defaultPaddings[0];
      if (paddingRight == 5555) paddingRight = data.defaultPaddings[1];
      if (paddingTop == 5555) paddingTop = data.defaultPaddings[2];
      if (paddingBottom == 5555) paddingBottom = data.defaultPaddings[3];

      // Report padding values
      LogToFile(fmt::format(
          "Display {} Padding -> {:>5}{}, {:>5}{}, {:>5}{}, {:>5}{}", i++,
          data.defaultPaddings[0], (paddingLeft == 5555) ? "(default)" : "",
          data.defaultPaddings[1], (paddingRight == 5555) ? "(default)" : "",
          data.defaultPaddings[2], (paddingTop == 5555) ? "(default)" : "",
          data.defaultPaddings[3], (paddingBottom == 5555) ? "(default)" : ""));

      newProfile.bounds.push_back(
          CBounds({rotLeft, rotRight, rotTop, rotBottom},
                  {paddingLeft, paddingRight, paddingTop, paddingBottom}));
    }

    data.profiles.push_back(newProfile);
    // extra line
  }
  return;
}

// clang-format off
void CConfig::SaveSettings() {
  const std::string FileName = "settings_test.toml";

  toml::value general{
        {"track_on_start", data.trackOnStart},
        {"quit_on_loss_of_track_ir", data.quitOnLossOfTrackIr},
        {"watchdog_enabled", data.watchdogEnabled},
        {"trackir_dll_directory", data.trackIrDllFolder},
        {"active_profile", data.activeProfileName},
    };

  toml::value padding{
    {"left", data.defaultPaddings[0]},
    {"right", data.defaultPaddings[1]},
    {"top", data.defaultPaddings[2]},
    {"bottom", data.defaultPaddings[3]}
  };

  std::vector<toml::value> profiles;
  for(auto& profile : data.profiles){
    std::vector<toml::value> displays;
    for(auto& display : profile.bounds){
        toml::value d{
          {"left", display.rotationBounds[0]},
          {"right", display.rotationBounds[1]},
          {"top", display.rotationBounds[2]},
          {"bottom", display.rotationBounds[3]}
        };
      displays.push_back(d);
    }
    toml::value top{
      {"name", profile.name},
      {"profile_id", profile.profile_ID},
      {"use_default_padding", profile.useDefaultPadding},
      {"DisplayMappings", displays}
    };
    profiles.push_back(top);
  }

  toml::value topTable{
    {"General", general},
    {"DefaultPadding", padding},
    {"Profiles", profiles},
  };

  // An example of how to access the underlying std::unordered_map<>
  // of a toml table. New or existing keyvalue pairs can be modified this way.
  // data.as_table()["a"] = a;

  std::fstream file(FileName, std::ios_base::out);
  file << topTable << std::endl;
  file.close();
}
// clang-format on

void CConfig::LogTomlError(const std::exception &ex) {
  wxLogFatalError("Incorrect type on reading configuration parameter: %s",
                  ex.what());
}

void CConfig::AddProfile(std::string newProfileName) {
  auto profileNames = GetProfileNames();

  // Prohibit conflicting names
  for (auto name : profileNames) {
    if (name == newProfileName) {
      wxLogError("Profile name already exists, please pick another name.");
      return;
    }
  }

  // TODO: optimize to emplace back as rvalue
  // Default profile
  SProfile p;
  p.name = newProfileName;
  data.profiles.push_back(p);
}

void CConfig::RemoveProfile(std::string profileName) {
  // find profile with name in profile table
  // delete the key, value pair
  // TODO: need to make profile names mututally exclusive
  // returns a vector

  for (std::size_t i = 0; i < data.profiles.size(); i++) {
    if (profileName == data.profiles[i].name) {
      data.profiles.erase(data.profiles.begin() + i);
      LogToWix(fmt::format("Deleted profile: {}\n", profileName));
      LogToFile(fmt::format("Deleted profile: {}", profileName));
    }
  }
}

void CConfig::DuplicateActiveProfile()
{
  auto profile = GetActiveProfile();
  profile.name.append("2"); // incremental profile name
  data.profiles.push_back(profile);
  data.activeProfileName = profile.name;
}

std::vector<std::string> CConfig::GetProfileNames() {
  std::vector<std::string> profileNames;
  for (auto &profile : data.profiles) {
    profileNames.push_back(profile.name);
  }
  return profileNames;
}

SProfile &CConfig::GetActiveProfile() {
  LogToFile(fmt::format("activeProfileName: {}", data.activeProfileName));
  for (auto &profile : data.profiles) {
    LogToFile(fmt::format("profile name: {}", profile.name));
    if (profile.name == data.activeProfileName) {
      return profile;
    }
  }
  // a profile should exist, codes shouldn't be reachable
  wxFAIL_MSG("Couldn't find active profile by name.");
}

int CConfig::GetActiveProfileDisplayCount() {
  SProfile profile = GetActiveProfile();
  return profile.bounds.size();
}

void CConfig::SetDisplayMappingParameter(int displayNumber, int parameterType,
                                         int parameterSide, float parameter) {}
