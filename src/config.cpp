/**
 * Singleton holding user & environment userData.
 * Loads user data from json file and environment data from registry and other
 * system calls.
 *
 * --License Boilerplate Placeholder--
 */

#include "config.hpp"

#include <Windows.h>

#include <iostream>
#include <string>

#include "constants.hpp"
#include "exceptions.hpp"
#include "log.hpp"

#define TOML11_PRESERVE_COMMENTS_BY_DEFAULT
#include "toml.hpp"

namespace config {

static UserData userData;
static EnvData environmentData;

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

UserData GetUserData() { return userData; }

UserData &GetUserDataMutable() { return userData; }

EnvData GetEnvironmentData() { return environmentData; }
// EnvData &GetEnvironmentDataMutable();

// Getter functions
// Profile GetActiveProfile();
// Profile &GetActiveProfile();

/**
 * Loads json user data into config class.
 * @return error_code
 */
void LoadSettingsFromFile(const std::string fileName) {
  // hungarian prefix tv___ = toml::value
  auto tvData = toml::parse<toml::preserve_comments>(fileName);

  environmentData.monitorCount = GetSystemMetrics(SM_CMONITORS);

  // Find the general settings table
  auto &vGeneralSettings = toml::find(tvData, "General");

  // Verify values exist and parse to correct type
  userData.trackOnStart = toml::find<bool>(vGeneralSettings, "track_on_start");
  userData.quitOnLossOfTrackIr =
      toml::find<bool>(vGeneralSettings, "quit_on_loss_of_track_ir");
  userData.watchdogEnabled =
      toml::find<bool>(vGeneralSettings, "watchdog_enabled");

  // Optionally the user can specify the location to the trackIR dll
  userData.trackIrDllFolder =
      toml::find<std::string>(vGeneralSettings, "trackir_dll_directory");

  userData.activeProfileName =
      toml::find<std::string>(vGeneralSettings, "active_profile");

  //////////////////////////////////////////////////////////////////////
  //                  Finding NPTrackIR DLL Location                  //
  //////////////////////////////////////////////////////////////////////
  std::string dllpath;

  if ("default" == userData.trackIrDllFolder) {
    RegistryQuery path = GetStringFromRegistry(
        HKEY_CURRENT_USER,
        "Software\\NaturalPoint\\NATURALPOINT\\NPClient Location", "Path");

    if (0 == path.result) {
      dllpath = path.value;
      spdlog::info("Acquired DLL location from registry.");
    } else {
      spdlog::error(
          "Could not find registry path\n  path.result: {}"
          "  path.resultString: {}",
          path.result, path.resultString);
    }
  } else {
    dllpath = userData.trackIrDllFolder;
  }

  // Check if DLL folder path is postfixed with slashes
  if (dllpath.back() != '\\') {
    dllpath.push_back('\\');
  }

// Match to the correct bitness of this application
#if defined(_WIN64) || defined(__amd64__)
  dllpath.append("NPClient64.dll");
#else
  m_sTrackIrDllLocation.append("NPClient.dll");
#endif

  spdlog::debug("NPTrackIR DLL Path: {}", dllpath);
  environmentData.trackIrDllPath = dllpath;

  //////////////////////////////////////////////////////////////////////
  //                     Finding Default Padding                      //
  //////////////////////////////////////////////////////////////////////

  // Catch padding table errors and notify user, because reverting to e
  // 0 padding is not a critical to program function
  toml::value vDefaultPaddingTable;

  try {
    vDefaultPaddingTable = toml::find(tvData, "DefaultPadding");
  } catch (std::out_of_range e) {
    spdlog::warn("Default Padding Table Not Found");
    spdlog::warn(
        "Please add the following to the settings.toml file:\n"
        "[DefaultPadding]\n"
        "left   = 0\n"
        "right  = 0\n"
        "top    = 0\n"
        "bottom = 0");
  }

  userData.defaultPaddings[0] = toml::find<int>(vDefaultPaddingTable, "left");
  userData.defaultPaddings[1] = toml::find<int>(vDefaultPaddingTable, "right");
  userData.defaultPaddings[2] = toml::find<int>(vDefaultPaddingTable, "top");
  userData.defaultPaddings[3] = toml::find<int>(vDefaultPaddingTable, "bottom");

  //////////////////////////////////////////////////////////////////////
  //                      Find Profiles Mapping                       //
  //////////////////////////////////////////////////////////////////////

  // Find the profiles table that contains all mapping profiles.
  auto vProfilesArray = toml::find(tvData, "Profiles");

  int i = 0;
  for (auto profile : vProfilesArray.as_array()) {
    Profile newProfile;
    newProfile.name = toml::find<std::string>(profile, "name");
    newProfile.profileId = toml::find<int>(profile, "profile_id");
    newProfile.useDefaultPadding =
        toml::find<bool>(profile, "use_default_padding");

    // Find the display mapping table for the given profile
    auto &tvDisplayMapping = toml::find(profile, "DisplayMappings");

    for (auto &display : tvDisplayMapping.as_array()) {
      /** bring in the rotational bounds
       * further information
       */
      toml::value left = toml::find(display, "left");
      toml::value right = toml::find(display, "right");
      toml::value top = toml::find(display, "top");
      toml::value bottom = toml::find(display, "bottom");

      // Each value is checked because this toml library cannot
      // convert integers to doubles from a toml value
      toml::value_t integer = toml::value_t::integer;

      double rotLeft = (left.type() == integer)
                           ? static_cast<double>(left.as_integer())
                           : left.as_floating();

      double rotRight = (right.type() == integer)
                            ? static_cast<double>(right.as_integer())
                            : right.as_floating();

      double rotTop = (top.type() == integer)
                          ? static_cast<double>(top.as_integer())
                          : top.as_floating();

      double rotBottom = (bottom.type() == integer)
                             ? static_cast<double>(bottom.as_integer())
                             : bottom.as_floating();

      // I return an ungodly fake high padding numbelong ,
      // so that I can tell if one was found in the toml config file
      // without producing an exception if a value was not found.
      // Padding values are not critical the program operation.
      int paddingLeft = toml::find_or<int>(display, "pad_left", 5555);
      int paddingRight = toml::find_or<int>(display, "pad_right", 5555);
      int paddingTop = toml::find_or<int>(display, "pad_top", 5555);
      int paddingBottom = toml::find_or<int>(display, "pad_bottom", 5555);

      if (paddingLeft == 5555) paddingLeft = userData.defaultPaddings[0];
      if (paddingRight == 5555) paddingRight = userData.defaultPaddings[1];
      if (paddingTop == 5555) paddingTop = userData.defaultPaddings[2];
      if (paddingBottom == 5555) paddingBottom = userData.defaultPaddings[3];

      // Report padding values
      spdlog::debug(
          "Display {} Padding -> {}{}, {}{}, {}{}, {}{}", i++,
          userData.defaultPaddings[0], (paddingLeft == 5555) ? "(default)" : "",
          userData.defaultPaddings[1],
          (paddingRight == 5555) ? "(default)" : "",
          userData.defaultPaddings[2], (paddingTop == 5555) ? "(default)" : "",
          userData.defaultPaddings[3],
          (paddingBottom == 5555) ? "(default)" : "");

      newProfile.displays.push_back(
          {{rotLeft, rotRight, rotTop, rotBottom},
           {paddingLeft, paddingRight, paddingTop, paddingBottom}});
    }

    userData.profiles.push_back(newProfile);
  }
  return;
}

// clang-format off

void WriteSettingsToFile() {
  const std::string fileName = "settings.toml";

  toml::value general{
    {"track_on_start", userData.trackOnStart},
    {"quit_on_loss_of_track_ir", userData.quitOnLossOfTrackIr},
    {"watchdog_enabled", userData.watchdogEnabled},
    {"trackir_dll_directory", userData.trackIrDllFolder},
    {"active_profile", userData.activeProfileName},
  };

  toml::value padding{
    {"left", userData.defaultPaddings[0]},
    {"right", userData.defaultPaddings[1]},
    {"top", userData.defaultPaddings[2]},
    {"bottom", userData.defaultPaddings[3]}
  };

  std::vector<toml::value> profiles;
  // write out profile data
  for(auto& profile : userData.profiles){
    std::vector<toml::value> displays;
    for(auto& display : profile.displays){
        toml::value d{
          {"left", display.rotation[0]},
          {"right", display.rotation[1]},
          {"top", display.rotation[2]},
          {"bottom", display.rotation[3]},
          {"pad_left", display.padding[0]},
          {"pad_right", display.padding[1]},
          {"pad_top", display.padding[2]},
          {"pad_bottom", display.padding[3]},
        };
      // TODO: urgent, add padding data
      displays.push_back(d);
    }
    toml::value top{
      {"name", profile.name},
      {"profile_id", profile.profileId},
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
  // userData.as_table()["a"] = a;

  std::fstream file(fileName, std::ios_base::out);
  file << topTable << std::endl;
  file.close();
}
// clang-format on

bool SetActiveProfile(std::string profileName) {
  // check if a valid name
  auto profileNames = GetProfileNames();
  bool found = false;
  for (auto name : profileNames) {
    if (name == profileName) {
      found = true;
    }
  }
  if (!found) {
    spdlog::error("Profile could not be found.");
    return false;
  }

  userData.activeProfileName = profileName;
  return true;
}

void AddProfile(std::string newProfileName) {
  auto profileNames = GetProfileNames();

  // Prohibit conflicting names
  for (auto name : profileNames) {
    if (name == newProfileName) {
      spdlog::error("Profile name already exists, please pick another name.");
      return;
    }
  }

  Profile p;  // Creat default profile
  p.name = newProfileName;
  userData.profiles.push_back(p);
}

void RemoveProfile(std::string profileName) {
  for (std::size_t i = 0; i < userData.profiles.size(); i++) {
    if (profileName == userData.profiles[i].name) {
      userData.profiles.erase(userData.profiles.begin() + i);
      spdlog::info("Deleted profile: {}", profileName);
    }
  }
  // change the active profile if deleted
  if (userData.activeProfileName == profileName) {
    auto name = userData.profiles[0].name;
    SetActiveProfile(name);
  }
}

void DuplicateActiveProfile() {
  auto profile = GetActiveProfile();
  profile.name.append("2");  // incremental profile name
  userData.profiles.push_back(profile);
  userData.activeProfileName = profile.name;
}

std::vector<std::string> GetProfileNames() {
  std::vector<std::string> profileNames;
  for (auto &profile : userData.profiles) {
    profileNames.push_back(profile.name);
  }
  return profileNames;
}

Profile &GetActiveProfileMutable() {
  for (auto &profile : userData.profiles) {
    if (profile.name == userData.activeProfileName) {
      return profile;
    }
  }
  // a profile should exist, codes shouldn't be reachable
  spdlog::critical(
      "An internal error has occured. Couldn't find active profile by name.");
}
// TODO change this to const
Profile GetActiveProfile() {
  for (auto &profile : userData.profiles) {
    if (profile.name == userData.activeProfileName) {
      return profile;
    }
  }
  // a profile should exist, codes shouldn't be reachable
  spdlog::critical(
      "An internal error has occured. Couldn't find active profile by name.");
}

int GetActiveProfileDisplayCount() {
  return GetActiveProfile().displays.size();
}

void SetActProfDisplayMappingParam(int displayNumber, int parameterType,
                                   int parameterSide, double parameter) {}

bool ValidateUserInput(const UserInput &displays) {
  // compare bounds not more than abs|180|
  for (int i = 0; i < displays.size(); i++) {
    for (int j = 0; j < 4; j++) {
      double degrees = displays[i].rotation[j];
      int padding = displays[i].padding[j];
      if (degrees > 180.0 || degrees < -180.0) {
        spdlog::error(
            "rotation bound param \"{}\" on display #{} is outside "
            "allowable range of -180deg -> 180deg.",
            kBoundNames[j], i);
        return false;
      }
    }
  }
  // see if any rectangles overlap
  // visualization: https://silentmatt.com/rectangle-intersection/
  for (int i = 0; i < displays.size(); i++) {
    int j = i;
    while (++j < displays.size()) {
      double A_left = displays[i].rotation[0];
      double A_right = displays[i].rotation[1];
      double A_top = displays[i].rotation[2];
      double A_bottom = displays[i].rotation[3];
      double B_left = displays[j].rotation[0];
      double B_right = displays[j].rotation[1];
      double B_top = displays[j].rotation[2];
      double B_bottom = displays[j].rotation[3];
      if (A_left < B_right && A_right > B_left && A_top > B_bottom &&
          A_bottom < B_top) {
        spdlog::error(
            "Overlapping rotational bounds between display #{} and #{}", i, j);
        return false;
      }
    }
  }
  return true;
}
}  // namespace config
