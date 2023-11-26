/**
 * Singleton holding user & environment user_data.
 * Loads user data from json file and environment data from registry and other
 * system calls.
 *
 * --License Boilerplate Placeholder--
 */

#include "config.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>

#include "log.hpp"
#include "types.hpp"
#include "utility.hpp"

#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace config {

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserDisplay, rotation, padding)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Profile,
                                   name,
                                   profile_id,
                                   use_default_padding,
                                   displays)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserData,
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

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Config, user_data)

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

auto
InitializeFromFile() -> void
{
  std::ifstream f("settings.json");

  if (!f.good()) {
    throw std::exception("can't access file 'settings.json'.");
  }

  json j = json::parse(f);
  auto data = j.template get<Config>();
  Set(data);
}

LoadResults
LoadFromFile(std::string filename)
{
  try {
    InitializeFromFile();
    return LoadResults{ true, "" };
  } catch (const std::exception& ex) {
    // json::exception inherits from std::exception
    // build a default configuration object
    config::Set(config::Config());
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
Config::SaveToFile()
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
                      "allowable range of -180� -> 180�.",
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
