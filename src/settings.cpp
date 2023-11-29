/**
 * Singleton holding user & environment data
 * Loads user data from json file and environment data from registry and other
 * system calls.
 *
 * --License Boilerplate Placeholder--
 */

#include "json.hpp"
#include "settings.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>

#include "log.hpp"
#include "types.hpp"
#include "utility.hpp"

namespace settings {

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserDisplay, rotation, padding)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Profile,
                                   name,
                                   profile_id,
                                   use_default_padding,
                                   displays)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Settings,
                                   active_profile_name,
                                   track_on_start,
                                   quit_on_loss_of_trackir,
                                   auto_find_track_ir_dll,
                                   track_ir_dll_folder,
                                   pipe_server_enabled,
                                   pipe_server_name,
                                   hotkey_enabled,
                                   log_level,
                                   default_padding,
                                   profiles)

// settings singleton
static std::shared_ptr<Settings> g_config;

// TODO: a shared pointer will not work for thread safety
auto
Get() -> std::shared_ptr<Settings>
{
  return g_config;
}

// TODO: a shared pointer will not work for thread safety
auto
GetCopy() -> Settings
{
  return Settings(*g_config);
}

// attemp at some thread safety
auto
Set(const Settings c) -> void
{
  g_config = std::make_shared<Settings>(c);
}

LoadResults
LoadFromFile(std::string filename)
{
  try {
    auto settings = LoadJsonFromFileIntoObject<Settings>("settings.json");
    Set(settings);
    return LoadResults{ true, "" };
  } catch (const std::exception& ex) {
    // I catch any exceptions here because It gives me the flexibility to handle
    // library specific exceptions without exposing library.
    // json::exception inherits from std::exception build a default
    // configuration object

    // Make default settings.
    settings::Set(settings::Settings());
    return LoadResults{ false, ex.what() };
  }
}

/**
 * Save current user data to disk.
 * Builds a toml object.
 * Uses toml supplied serializer via ostream operators to write to file.
 */
// TODO: wrap this in exceptions
void
Settings::SaveToFile()
{
  json j = *(Get());
  std::ofstream f("settings.json");
  f << j.dump(4);
}

/**
 * set the profile with the given name as active
 * assumes active profile is always present within the vector of profiles
 */
bool
Settings::SetActiveProfile(std::string profile_name)
{
  // check if name exists in user data
  for (const auto& name : GetProfileNames()) {
    if (name == profile_name) {
      active_profile_name = profile_name;
      return true;
    }
  }
  spdlog::error("Profile could not be found.");
  return false;
}

// created default profile with the given name
void
Settings::AddProfile(std::string new_profile_name = "")
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
  profiles.push_back(p);

  SetActiveProfile(new_profile_name);
}

/**
 * remove profile with the given name from internal configuration
 */
void
Settings::RemoveProfile(std::string profile_name)
{
  // search for and remove desired profile
  for (int i = 0; i < profiles.size(); i++) {
    if (profile_name == profiles[i].name) {
      profiles.erase(profiles.begin() + i);
      spdlog::info("Deleted profile: {}", profile_name);
    }
  }

  // ensure at least one profile exists
  if (profiles.empty()) {
    profiles.emplace_back();
  }

  // change the active profile if the active profile was deleted
  if (active_profile_name == profile_name) {
    const auto& first = profiles[0].name;
    SetActiveProfile(first);
  }
}

// make a copy of the active profile with an arbitrary temporary name
void
Settings::DuplicateActiveProfile()
{
  auto new_profile = GetActiveProfile();
  new_profile.name.append("2"); // incremental profile name
  profiles.push_back(new_profile);
  active_profile_name = new_profile.name;
}

std::vector<std::string>
Settings::GetProfileNames()
{
  std::vector<std::string> profile_names;
  for (const auto& profile : profiles) {
    profile_names.push_back(profile.name);
  }
  return profile_names;
}

// return reference the underlying active profile
// assumes active profile is always present within the vector of profiles
Profile&
Settings::GetActiveProfile()
{
  for (auto& profile : profiles) {
    if (profile.name == active_profile_name) {
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
Settings::GetActiveProfileDisplayCount()
{
  return static_cast<int>(GetActiveProfile().displays.size());
}

void
Settings::SetActProfDisplayMappingParam(int display_number,
                                        int param_type,
                                        int param_side,
                                        double param)
{
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
      Degrees A_left = displays[i].rotation[LEFT_EDGE];
      Degrees A_right = displays[i].rotation[RIGHT_EDGE];
      Degrees A_top = displays[i].rotation[TOP_EDGE];
      Degrees A_bottom = displays[i].rotation[BOTTOM_EDGE];
      Degrees B_left = displays[j].rotation[LEFT_EDGE];
      Degrees B_right = displays[j].rotation[RIGHT_EDGE];
      Degrees B_top = displays[j].rotation[TOP_EDGE];
      Degrees B_bottom = displays[j].rotation[BOTTOM_EDGE];
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
bool
Profile::ValidateParameters() const
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
      Degrees A_left = displays[i].rotation[LEFT_EDGE];
      Degrees A_right = displays[i].rotation[RIGHT_EDGE];
      Degrees A_top = displays[i].rotation[TOP_EDGE];
      Degrees A_bottom = displays[i].rotation[BOTTOM_EDGE];
      Degrees B_left = displays[j].rotation[LEFT_EDGE];
      Degrees B_right = displays[j].rotation[RIGHT_EDGE];
      Degrees B_top = displays[j].rotation[TOP_EDGE];
      Degrees B_bottom = displays[j].rotation[BOTTOM_EDGE];
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
