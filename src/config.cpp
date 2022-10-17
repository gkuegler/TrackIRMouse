/**
 * Singleton holding user & environment user_data.
 * Loads user data from json file and environment data from registry and other
 * system calls.
 *
 * --License Boilerplate Placeholder--
 */

#include "config.hpp"

#include <Windows.h>

#include <iostream>
#include <string>

#include "log.hpp"
#include "types.hpp"

#define TOML11_PRESERVE_COMMENTS_BY_DEFAULT
#include "toml11/toml.hpp"

namespace config {

// settings singletons
static std::shared_ptr<Config> g_config;

// TODO: a shared pointer will not work for thread safety
std::shared_ptr<Config>
Get()
{
  return g_config;
}

// attemp at some thread safety
void
Set(const Config c)
{
  g_config = std::make_shared<Config>(c);
}

typedef struct RegistryQuery_
{
  int result;
  std::string result_string;
  std::string value;
} RegistryQuery;

/**
 * [GetStringFromRegistry description]
 * @param  parent_key [description]
 * @param  sub_key     [description]
 * @param  sub_value   [description]
 * @return            [description]
 */
RegistryQuery
GetStringFromRegistry(HKEY parent_key,
                      const char* sub_key,
                      const char* sub_value)
{
  //////////////////////////////////////////////////////////////////////
  //                         Opening The Key                          //
  //////////////////////////////////////////////////////////////////////

  HKEY hKey = 0;

  LSTATUS status_key_open =
    RegOpenKeyExA(parent_key, // should usually be HKEY_CURRENT_USER
                  sub_key,
                  0,        //[in]           DWORD  ulOptions,
                  KEY_READ, //[in]           REGSAM samDesired,
                  &hKey);

  if (ERROR_FILE_NOT_FOUND == status_key_open) {
    return RegistryQuery{ ERROR_FILE_NOT_FOUND, "Registry key not found.", "" };
  }

  // Catch all other errors
  if (ERROR_SUCCESS != status_key_open) {
    return RegistryQuery{ status_key_open, "Could not open registry key.", "" };
  }

  //////////////////////////////////////////////////////////////////////
  //                    Querying Value Information                    //
  //////////////////////////////////////////////////////////////////////

  DWORD value_type = 0;
  DWORD size_of_buffer = 0;

  LSTATUS query_status =
    RegQueryValueExA(hKey,           // [in]                HKEY    hKey,
                     "Path",         // [in, optional]      LPCSTR  lpValueName,
                     0,              // LPDWORD lpReserved,
                     &value_type,    // [out, optional]     LPDWORD lpType,
                     0,              // [out, optional]     LPBYTE  lpData,
                     &size_of_buffer // [in, out, optional] LPDWORD lpcbData
    );

  if (ERROR_FILE_NOT_FOUND == query_status) {
    return RegistryQuery{ ERROR_FILE_NOT_FOUND,
                          "Value not found for key.",
                          "" };
  }

  // Catch all other errors of RegQueryValueExA
  if (ERROR_SUCCESS != query_status) {
    return RegistryQuery{ query_status, "RegQueryValueExA failed.", "" };
  }

  if (REG_SZ != value_type) {
    return RegistryQuery{ 1, "Registry value not a string type.", "" };
  }

  //////////////////////////////////////////////////////////////////////
  //                      Getting the hKey Value                       //
  //////////////////////////////////////////////////////////////////////

  // Registry key may or may not be stored with a null terminator
  // add one just in case
  char* szPath = static_cast<char*>(calloc(1, size_of_buffer + 1));

  if (NULL == szPath) {
    return RegistryQuery{ 1, "Failed to allocate memory.", "" };
  }

  LSTATUS status_get_value =
    RegGetValueA(hKey,           // [in]                HKEY    hkey,
                 0,              // [in, optional]      LPCSTR  lpSubKey,
                 sub_value,      // [in, optional]      LPCSTR  lpValue,
                 RRF_RT_REG_SZ,  // [in, optional]      DWORD   dwFlags,
                 &value_type,    // [out, optional]     LPDWORD pdwType,
                 (void*)szPath,  // [out, optional]     PVOID   pvData,
                 &size_of_buffer // [in, out, optional] LPDWORD pcbData
    );

  if (ERROR_SUCCESS == status_get_value) {
    return RegistryQuery{ 0, "", std::string(szPath) };
  } else {
    return RegistryQuery{ status_get_value, "Could not get registry key.", "" };
  }
}

/**
 * Loads toml user data into config class.
 * Will throw detailed errors from toml library if any of the following occur
 *   - std::runtime_error - failed to open file
 *   - toml::syntax_error - failed to parse file into toml object
 *   - toml::type_error - failed conversion from 'toml::find'
 *   - std::out_of_range - a table or value is not found
 */
Config::Config(const std::string filename)
{
  // note: hungarian notation prefix tv___ = toml::value
  filename_ = filename;
  const auto t_data = toml::parse<toml::preserve_comments>(filename);
  env_data.monitor_count = GetSystemMetrics(SM_CMONITORS);

  // Find the general settings table
  const auto& t_general_settings = toml::find(t_data, "General");

  // Verify values exist and parse to correct type
  user_data.track_on_start =
    toml::find<bool>(t_general_settings, "track_on_start");
  user_data.quit_on_loss_of_trackir =
    toml::find<bool>(t_general_settings, "quit_on_loss_of_track_ir");
  user_data.watchdog_enabled =
    toml::find<bool>(t_general_settings, "watchdog_enabled");
  user_data.log_level = static_cast<spdlog::level::level_enum>(
    toml::find<int>(t_general_settings, "log_level"));
  user_data.auto_find_track_ir_dll =
    toml::find<bool>(t_general_settings, "auto_find_trackir_dll");

  // Optionally the user can specify the location to the trackIR dll
  user_data.track_ir_dll_folder =
    toml::find<std::string>(t_general_settings, "trackir_dll_directory");

  user_data.active_profile_name =
    toml::find<std::string>(t_general_settings, "active_profile");

  // TODO: implement this in settings file
  user_data.auto_find_track_ir_dll = true;

  //////////////////////////////////////////////////////////////////////
  //                  Finding NPTrackIR DLL Location                  //
  //////////////////////////////////////////////////////////////////////
  std::string dll_path;

  if (user_data.auto_find_track_ir_dll) {
    RegistryQuery path = GetStringFromRegistry(
      HKEY_CURRENT_USER,
      "Software\\NaturalPoint\\NATURALPOINT\\NPClient Location",
      "Path");

    if (0 == path.result) {
      dll_path = path.value;
      spdlog::info("Acquired DLL location from registry.");
    } else {
      spdlog::error(
        "Could not find registry path. NP TrackIR may not be installed. To "
        "fix error, try specifying the folder of the \"NPClient64.dll\" in "
        "edit->settings.");
      spdlog::info("path.result: {}\n"
                   "path.resultString: {}",
                   path.result,
                   path.result_string);
    }
  } else {
    dll_path = user_data.track_ir_dll_folder;
  }

  // Check if DLL folder path is postfixed with slashes
  if (dll_path.back() != '\\') {
    dll_path.push_back('\\');
  }

// Match to the correct bitness of this application
#if defined(_WIN64) || defined(__amd64__)
  dll_path.append("NPClient64.dll");
#else
  dll_path.append("NPClient.dll");
#endif

  spdlog::debug("NPTrackIR DLL Path: {}", dll_path);
  env_data.track_ir_dll_path = dll_path;

  //////////////////////////////////////////////////////////////////////
  //                     Finding Default Padding                      //
  //////////////////////////////////////////////////////////////////////

  // Catch padding table errors and notify user, because reverting to e
  // 0 padding is not a critical to program function
  toml::value default_padding_table;

  try {
    default_padding_table = toml::find(t_data, "DefaultPadding");
  } catch (std::out_of_range e) {
    spdlog::warn("Default Padding Table Not Found");
    spdlog::warn("Please add the following to the settings.toml file:\n"
                 "[DefaultPadding]\n"
                 "left   = 0\n"
                 "right  = 0\n"
                 "top    = 0\n"
                 "bottom = 0");
  }

  user_data.default_padding[0] = toml::find<int>(default_padding_table, "left");
  user_data.default_padding[1] =
    toml::find<int>(default_padding_table, "right");
  user_data.default_padding[2] = toml::find<int>(default_padding_table, "top");
  user_data.default_padding[3] =
    toml::find<int>(default_padding_table, "bottom");

  //////////////////////////////////////////////////////////////////////
  //                      Find Profiles Mapping                       //
  //////////////////////////////////////////////////////////////////////

  user_data.profiles = {}; // clear default profile

  // Find the profiles table that contains all mapping profiles.
  const auto& profiles = toml::find(t_data, "Profiles").as_array();
  for (int i = 0; i < profiles.size(); i++) {
    const auto& profile = profiles[i];
    Profile new_profile;
    new_profile.name = toml::find<std::string>(profile, "name");
    new_profile.profile_id = toml::find<int>(profile, "profile_id");
    new_profile.use_default_padding =
      toml::find<bool>(profile, "use_default_padding");

    // Find the display mapping table for the given profile
    auto& t_display_mapping = toml::find(profile, "DisplayMappings");

    for (auto& display : t_display_mapping.as_array()) {
      // bring in the rotational bounds
      toml::value left = toml::find(display, "left");
      toml::value right = toml::find(display, "right");
      toml::value top = toml::find(display, "top");
      toml::value bottom = toml::find(display, "bottom");

      // Each value is checked because this toml library cannot
      // convert integers to doubles from a toml value
      const Degrees deg_left = (left.type() == toml::value_t::integer)
                                 ? static_cast<Degrees>(left.as_integer())
                                 : static_cast<Degrees>(left.as_floating());

      const Degrees deg_right = (right.type() == toml::value_t::integer)
                                  ? static_cast<Degrees>(right.as_integer())
                                  : static_cast<Degrees>(right.as_floating());

      const Degrees deg_top = (top.type() == toml::value_t::integer)
                                ? static_cast<Degrees>(top.as_integer())
                                : static_cast<Degrees>(top.as_floating());

      const Degrees deg_bottom = (bottom.type() == toml::value_t::integer)
                                   ? static_cast<Degrees>(bottom.as_integer())
                                   : static_cast<Degrees>(bottom.as_floating());

      // I return an ungodly fake high padding numbelong,
      // so that I can tell if one was found in the toml config file
      // without producing an exception if a value was not found.
      // Padding values are not critical the program operation.
      Pixels pleft = toml::find_or<Pixels>(display, "pad_left", 5555);
      Pixels pright = toml::find_or<Pixels>(display, "pad_right", 5555);
      Pixels ptop = toml::find_or<Pixels>(display, "pad_top", 5555);
      Pixels pbottom = toml::find_or<Pixels>(display, "pad_bottom", 5555);

      if (pleft == 5555)
        pleft = user_data.default_padding[0];
      if (pright == 5555)
        pright = user_data.default_padding[1];
      if (ptop == 5555)
        ptop = user_data.default_padding[2];
      if (pbottom == 5555)
        pbottom = user_data.default_padding[3];

      // Report padding values
      spdlog::debug("Display {} Padding -> {}{}, {}{}, {}{}, {}{}",
                    i,
                    user_data.default_padding[0],
                    (pleft == 5555) ? "(default)" : "",
                    user_data.default_padding[1],
                    (pright == 5555) ? "(default)" : "",
                    user_data.default_padding[2],
                    (ptop == 5555) ? "(default)" : "",
                    user_data.default_padding[3],
                    (pbottom == 5555) ? "(default)" : "");

      new_profile.displays.push_back(
        { { deg_left, deg_right, deg_top, deg_bottom },
          { pleft, pright, ptop, pbottom } });
    }

    user_data.profiles.push_back(new_profile);
  }

  // validate that active profile exists in profiles list
  if (user_data.profiles.size() > 0) {
    bool found = false;
    for (auto& profile : user_data.profiles) {
      if (profile.name == user_data.active_profile_name) {
        found = true;
      }
    }
    if (!found) {
      // arbitrarily pick the first profile in the list
      user_data.active_profile_name = user_data.profiles[0].name;
    }
  } else { // append default profile to list if empty
    user_data.profiles = { { "empty", 0, true, {} } };
    user_data.active_profile_name = "empty";
  }

  return;
}

// clang-format off

/**
 * Save current user data to disk.
 * Builds a toml object.
 * Uses toml supplied serializer via ostream operators to write to file.
 */
void Config::save_to_file(std::string filename) {
  const toml::value general{
    {"track_on_start", user_data.track_on_start},
    {"quit_on_loss_of_track_ir", user_data.quit_on_loss_of_trackir},
    {"auto_find_trackir_dll", user_data.auto_find_track_ir_dll},
    {"trackir_dll_directory", user_data.track_ir_dll_folder},
    {"watchdog_enabled", user_data.watchdog_enabled},
		{"pipe_server_name", user_data.pipe_server_name},
    {"active_profile", user_data.active_profile_name},
    {"hotkey_enabled", user_data.hotkey_enabled},
    {"log_level", static_cast<int>(user_data.log_level)},
  };

  const toml::value padding{
    {"left", user_data.default_padding[0]},
    {"right", user_data.default_padding[1]},
    {"top", user_data.default_padding[2]},
    {"bottom", user_data.default_padding[3]}
  };

  // toml library has overloads for creating arrays from std::vector
  std::vector<toml::value> profiles;
  // write out profile data
  for(auto& profile : user_data.profiles){
    std::vector<toml::value> displays;
    for(auto& display : profile.displays){
        const toml::value d{
          {"left", display.rotation[0]},
          {"right", display.rotation[1]},
          {"top", display.rotation[2]},
          {"bottom", display.rotation[3]},
          {"pad_left", display.padding[0]},
          {"pad_right", display.padding[1]},
          {"pad_top", display.padding[2]},
          {"pad_bottom", display.padding[3]},
        };
      displays.push_back(d);
    }
    const toml::value top{
      {"name", profile.name},
      {"profile_id", profile.profile_id},
      {"use_default_padding", profile.use_default_padding},
      {"DisplayMappings", displays}
    };
    profiles.push_back(top);
  }

  const toml::value top_table{
    {"General", general},
    {"DefaultPadding", padding},
    {"Profiles", profiles},
  };

  std::fstream file(filename, std::ios_base::out);
  file << top_table << std::endl;
  file.close();
}
// clang-format on

// set the profile with the given name as active
// assumes active profile is always present within the vector of profiles
bool
Config::SetActiveProfile(std::string profile_name)
{
  // check if name exists in user data
  for (const auto& name : GetProfileNames()) {
    if (name == profile_name) {
      user_data.active_profile_name = profile_name;
      return true;
    }
  }
  spdlog::error("Profile could not be found.");
  return false;
}

// created default profile with the given name
void
Config::AddProfile(std::string new_profile_name = "")
{

  // Prohibit conflicting names
  for (const auto& name : GetProfileNames()) {
    if (name == new_profile_name) {
      spdlog::error("Profile name already exists, please pick another name.");
      return;
    }
  }

  // TODO: dont create and set later
  Profile p; // Creat default profile

  // otherwise use default name
  if (new_profile_name != "") {
    p.name = new_profile_name;
  }
  user_data.profiles.push_back(p);

  SetActiveProfile(new_profile_name);
}

// remove profile with the given name from internal configuration
void
Config::RemoveProfile(std::string profile_name)
{
  for (int i = 0; i < user_data.profiles.size(); i++) {
    if (profile_name == user_data.profiles[i].name) {
      user_data.profiles.erase(user_data.profiles.begin() + i);
      spdlog::info("Deleted profile: {}", profile_name);
    }
  }
  if (user_data.profiles.size() < 1) {
    user_data.profiles.emplace_back();
  }
  // change the active profile if deleted
  if (user_data.active_profile_name == profile_name) {
    auto first = user_data.profiles[0].name;
    SetActiveProfile(first);
  }
}

// make a copy of the active profile with an arbitrary temporary name
void
Config::DuplicateActiveProfile()
{
  auto new_profile = GetActiveProfile();
  new_profile.name.append("2"); // incremental profile name
  user_data.profiles.push_back(new_profile);
  user_data.active_profile_name = new_profile.name;
}

std::vector<std::string>
Config::GetProfileNames()
{
  std::vector<std::string> profile_names;
  for (const auto& profile : user_data.profiles) {
    profile_names.push_back(profile.name);
  }
  return profile_names;
}

// return reference the underlying active profile
// assumes active profile is always present within the vector of profiles
Profile&
Config::GetActiveProfile()
{
  for (auto& profile : user_data.profiles) {
    if (profile.name == user_data.active_profile_name) {
      return profile;
    }
  }
  // an active profile should exist, code shouldn't be reachable
  // design interface accordingly
  spdlog::critical(
    "An internal error has occured. Couldn't find active profile by name.");
}

// get the number of displays specified in the active profile
int
Config::GetActiveProfileDisplayCount()
{
  return static_cast<int>(GetActiveProfile().displays.size());
}

void
Config::SetActProfDisplayMappingParam(int display_number,
                                      int param_type,
                                      int param_side,
                                      double param)
{
}

ConfigReturn
LoadFromFile(std::string filename)
{
  std::string err_msg = "lorem ipsum";
  try {
    auto config = Config(filename);
    // return a successfully parsed and validated config file
    return ConfigReturn{ retcode::success, "", config };
  } catch (const toml::syntax_error& ex) {
    err_msg = fmt::format(
      "Syntax error in toml file: \"{}\"\nSee error message below for hints "
      "on how to fix.\n{}",
      filename,
      ex.what());
  } catch (const toml::type_error& ex) {
    err_msg = fmt::format("Incorrect type when parsing toml file \"{}\".\n\n{}",
                          filename,
                          ex.what());
  } catch (const std::out_of_range& ex) {
    err_msg = fmt::format(
      "Missing data in toml file \"{}\".\n\n{}", filename, ex.what());
  } catch (const std::runtime_error& ex) {
    err_msg = fmt::format("Failed to open \"{}\"", filename);
  } catch (...) {
    err_msg = fmt::format(
      "Exception has gone unhandled loading \"{}\" and verifying values.",
      filename);
  }

  // return a default config object with a message explaining failure
  return ConfigReturn{ retcode::fail, err_msg, Config() };
}

// load game titles by ID number from file
game_title_map_t
GetTitleIds()
{
  // loading from file allows the game titles to be modified for future
  // natural point continually adds new titles
  constexpr auto filename = "track-ir-numbers.toml";
  const std::string instructions =
    "Couldn't load game titles from file. See above error for how to fix.\nA "
    "sample of the title list will be loaded by default.\nUse "
    "\"File->Reload\" to fix the error and reload list.";
  std::string err_msg = "lorem ipsum";

  try {
    // load, parse, and return as map
    auto data = toml::parse(filename);
    return toml::find<game_title_map_t>(data, "data");
  } catch (const toml::syntax_error& ex) {
    err_msg = fmt::format(
      "Syntax error in toml file: \"%s\"\nSee error message below for hints "
      "on how to fix.\n%s",
      filename,
      ex.what());
  } catch (const toml::type_error& ex) {
    err_msg = fmt::format("Incorrect type when parsing toml file \"%s\".\n\n%s",
                          filename,
                          ex.what());
  } catch (const std::out_of_range& ex) {
    err_msg = fmt::format(
      "Missing data in toml file \"%s\".\n\n%s", filename, ex.what());
  } catch (std::runtime_error& ex) {
    err_msg = fmt::format("Failed to open \"%s\"", filename);
  } catch (...) {
    err_msg = fmt::format(
      "exception has gone unhandled loading \"%s\" and verifying values.",
      filename);
  }

  spdlog::error("{}\n\n{}", err_msg, instructions);

  // provide sample list since full list couldn't be loaded from file
  game_title_map_t sample_map;
  sample_map["1001"] = "IL-2 Forgotten Battles";
  sample_map["1002"] = "Lock-On Modern Air";
  sample_map["1003"] = "Black Shark";
  sample_map["1004"] = "Tom Clancy's H.A.W.X.";
  sample_map["1005"] = "LockOn: Flaming Cliffs 2";
  sample_map["1006"] = "DCS: A-10C";
  sample_map["1007"] = "Tom Clancy's H.A.W.X.";
  sample_map["1008"] = "IL-2 Struremovik: Battle";
  sample_map["1009"] = "The Crew";
  sample_map["1025"] = "Down In Flames";
  return sample_map;
}

bool
ValidateUserInput(const UserInput& displays)
{
  // compare bounds not more than abs|180|
  for (int i = 0; i < displays.size(); i++) {
    for (int j = 0; j < 4; j++) {
      auto d = displays[i].rotation[j];
      auto padding = displays[i].padding[j];
      if (d > 180.0 || d < -180.0) {
        spdlog::error("rotation bound param \"{}\" on display #{} is outside "
                      "allowable range of -180° -> 180°.",
                      k_edge_names[j],
                      i);
        return false;
      }
    }
  }
  // see if any rectangles overlap
  // visualization: https://silentmatt.com/rectangle-intersection/
  for (int i = 0; i < displays.size(); i++) {
    int j = i;
    while (++j < displays.size()) {
      Degrees A_left = displays[i].rotation[0];
      Degrees A_right = displays[i].rotation[1];
      Degrees A_top = displays[i].rotation[2];
      Degrees A_bottom = displays[i].rotation[3];
      Degrees B_left = displays[j].rotation[0];
      Degrees B_right = displays[j].rotation[1];
      Degrees B_top = displays[j].rotation[2];
      Degrees B_bottom = displays[j].rotation[3];
      if (A_left < B_right && A_right > B_left && A_top > B_bottom &&
          A_bottom < B_top) {
        spdlog::error(
          "Overlapping rotational bounds between display #{} and #{}", i, j);
        return false;
      }
    }
  }
  spdlog::trace("user input validated");
  return true;
}
} // namespace config
