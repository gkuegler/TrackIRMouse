/**
 * Singleton holding user & environment user_data.
 * Loads user data from json file and environment data from registry and other
 * system calls.
 *
 * --License Boilerplate Placeholder--
 */

#include "config.hpp"

#include <filesystem>
#include <iostream>
#include <string>

#include "log.hpp"
#include "types.hpp"
#include "util.hpp"

#define TOML11_PRESERVE_COMMENTS_BY_DEFAULT
#include "toml.hpp"
#include "toml/exception.hpp"

namespace config {

// settings singleton
static std::shared_ptr<Config> g_config;

// TODO: a shared pointer will not work for thread safety
auto
Get() -> std::shared_ptr<Config>
{
  return g_config;
}

// attemp at some thread safety
auto
Set(const Config c) -> void
{
  g_config = std::make_shared<Config>(c);
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

  // note: prefix t_* = toml::value
  file_path_ = util::GetExecutableFolder() + "\\" + filename;
  const auto t_data = toml::parse<toml::preserve_comments>(file_path_);

  // Find the general settings table
  const auto& t_general = toml::find(t_data, "General");

  // TODO: consider that default values are now being specified in 2 places
  // Verify values exist and parse to correct type
  UserData user = {
    toml::find<std::string>(t_general, "active_profile"),
    toml::find_or<bool>(t_general, "track_on_start", false),
    toml::find_or<bool>(t_general, "quit_on_loss_of_track_ir", false),
    toml::find_or<bool>(t_general, "auto_find_trackir_dll", true),
    toml::find<std::string>(t_general, "trackir_dll_directory"),
    toml::find_or<bool>(t_general, "pipe_server_enabled", false),
    toml::find_or<std::string>(t_general, "pipe_server_name", "watchdog"),
    toml::find_or<bool>(t_general, "hotkey_enabled", false),
    static_cast<spdlog::level::level_enum>(
      toml::find_or<int>(t_general, "log_level", spdlog::level::info))
  };
  // TODO: what to do when no active profile specified
  // TODO: convert log level in settings file to string

  //////////////////////////////////////////////////////////////////////
  //                     Finding Default Padding                      //
  //////////////////////////////////////////////////////////////////////

  // Catch padding table errors and notify user, because reverting to e
  // 0 padding is not a critical to program function
  toml::value t_default_padding;

  try {
    t_default_padding = toml::find(t_data, "DefaultPadding");
  } catch (std::out_of_range e) {
    spdlog::warn("Default Padding Table Not Found");
    spdlog::warn("Please add the following to the settings.toml file:\n"
                 "[DefaultPadding]\n"
                 "left   = 0\n"
                 "right  = 0\n"
                 "top    = 0\n"
                 "bottom = 0");
  }

  user.default_padding[LEFT_EDGE] = toml::find<int>(t_default_padding, "left");
  user.default_padding[RIGHT_EDGE] =
    toml::find<int>(t_default_padding, "right");
  user.default_padding[TOP_EDGE] = toml::find<int>(t_default_padding, "top");
  user.default_padding[BOTTOM_EDGE] =
    toml::find<int>(t_default_padding, "bottom");

  //////////////////////////////////////////////////////////////////////
  //                      Find Profiles Mapping                       //
  //////////////////////////////////////////////////////////////////////

  // user_data.profiles = {}; // clear default profile

  // Find the profiles table that contains all mapping profiles.
  const auto& profiles = toml::find(t_data, "Profiles").as_array();
  for (int i = 0; i < profiles.size(); i++) {
    const auto& profile = profiles[i];
    Profile new_profile = {
      toml::find<std::string>(profile, "name"),
      toml::find<int>(profile, "profile_id"),
      toml::find<bool>(profile, "use_default_padding"),
    }; // partial initialization

    // Find the display mapping table for the given profile
    auto& t_display_mapping = toml::find(profile, "DisplayMappings");

    for (auto& display : t_display_mapping.as_array()) {
      // find the rotational bounds
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

      // Return the padding rectangle specific to the display.
      // To avoid exceptions, I return an ungodly fake high padding numbe,
      // so that I can tell if one was found in the toml config file
      // without producing an exception if a value was not found.
      // This enables the user to specify individual padding values without
      // having to specify all or any of them.
      // Padding values are not critical the program operation.

      Pixels pleft = toml::find_or<Pixels>(display, "pad_left", 5555);
      Pixels pright = toml::find_or<Pixels>(display, "pad_right", 5555);
      Pixels ptop = toml::find_or<Pixels>(display, "pad_top", 5555);
      Pixels pbottom = toml::find_or<Pixels>(display, "pad_bottom", 5555);

      if (pleft == 5555)
        pleft = user.default_padding[LEFT_EDGE];
      if (pright == 5555)
        pright = user.default_padding[RIGHT_EDGE];
      if (ptop == 5555)
        ptop = user.default_padding[TOP_EDGE];
      if (pbottom == 5555)
        pbottom = user.default_padding[BOTTOM_EDGE];

      // Report padding values
      spdlog::trace("Display {} Padding -> {}{}, {}{}, {}{}, {}{}",
                    i,
                    user.default_padding[LEFT_EDGE],
                    (pleft == 5555) ? "(default)" : "",
                    user.default_padding[RIGHT_EDGE],
                    (pright == 5555) ? "(default)" : "",
                    user.default_padding[TOP_EDGE],
                    (ptop == 5555) ? "(default)" : "",
                    user.default_padding[BOTTOM_EDGE],
                    (pbottom == 5555) ? "(default)" : "");

      new_profile.displays.push_back(
        { { deg_left, deg_right, deg_top, deg_bottom },
          { pleft, pright, ptop, pbottom } });
    }

    user.profiles.push_back(new_profile);
  }

  // validate that active profile exists in profiles list
  if (user.profiles.size() > 0) {
    bool found = false;
    for (auto& profile : user.profiles) {
      if (profile.name == user.active_profile_name) {
        found = true;
      }
    }
    if (!found) {
      // arbitrarily pick the first profile in the list
      user.active_profile_name = user.profiles[0].name;
    }
  } else { // append default profile to list if empty
    user.profiles = { Profile() };
    user.active_profile_name = user.profiles[0].name;
  }

  user_data = user;

  return;
}

// clang-format off

/**
 * Save current user data to disk.
 * Builds a toml object.
 * Uses toml supplied serializer via ostream operators to write to file.
 */
void Config::SaveToFile() {
  const toml::value general{
    {"active_profile", user_data.active_profile_name},
    {"track_on_start", user_data.track_on_start},
    {"quit_on_loss_of_track_ir", user_data.quit_on_loss_of_trackir},
    {"auto_find_trackir_dll", user_data.auto_find_track_ir_dll},
    {"trackir_dll_directory", user_data.track_ir_dll_folder},
    {"pipe_server_enabled", user_data.pipe_server_enabled},
		{"pipe_server_name", user_data.pipe_server_name},
    {"hotkey_enabled", user_data.hotkey_enabled},
    {"log_level", static_cast<int>(user_data.log_level)},
  };

  const toml::value padding{
    {"left", user_data.default_padding[LEFT_EDGE]},
    {"right", user_data.default_padding[RIGHT_EDGE]},
    {"top", user_data.default_padding[TOP_EDGE]},
    {"bottom", user_data.default_padding[BOTTOM_EDGE]}
  };

  // toml library has overloads for creating arrays from std::vector
  std::vector<toml::value> profiles;
  // write out profile data
  for(auto& profile : user_data.profiles){
    std::vector<toml::value> displays;
    for(auto& display : profile.displays){
        const toml::value d{
          {"left", display.rotation[LEFT_EDGE]},
          {"right", display.rotation[RIGHT_EDGE]},
          {"top", display.rotation[TOP_EDGE]},
          {"bottom", display.rotation[BOTTOM_EDGE]},
          {"pad_left", display.padding[LEFT_EDGE]},
          {"pad_right", display.padding[RIGHT_EDGE]},
          {"pad_top", display.padding[TOP_EDGE]},
          {"pad_bottom", display.padding[BOTTOM_EDGE]},
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

  std::fstream file(file_path_, std::ios_base::out);
  file << top_table << std::endl;
  file.close();
}
// clang-format on

/**
 * set the profile with the given name as active
 * assumes active profile is always present within the vector of profiles
 */
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

  Profile p; // Creat default profile

  // otherwise use default name
  if (new_profile_name != "") {
    p.name = new_profile_name;
  }
  user_data.profiles.push_back(p);

  SetActiveProfile(new_profile_name);
}

/**
 * remove profile with the given name from internal configuration
 */
void
Config::RemoveProfile(std::string profile_name)
{
  // search for and remove desired profile
  for (int i = 0; i < user_data.profiles.size(); i++) {
    if (profile_name == user_data.profiles[i].name) {
      user_data.profiles.erase(user_data.profiles.begin() + i);
      spdlog::info("Deleted profile: {}", profile_name);
    }
  }

  // ensure at least one profile exists
  if (user_data.profiles.empty()) {
    user_data.profiles.emplace_back();
  }

  // change the active profile if the active profile was deleted
  if (user_data.active_profile_name == profile_name) {
    const auto& first = user_data.profiles[0].name;
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

} // namespace config
