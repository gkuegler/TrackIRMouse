#pragma once
#ifndef TIRMOUSE_CONFIG_HPP
#define TIRMOUSE_CONFIG_HPP

#include <array>
#include <map>
#include <string>

#include "log.hpp"
#include "types.hpp"

namespace settings {

// TODO: Move global to types?
struct UserDisplay
{
  UserDisplay(){};
  UserDisplay(RectDegrees r, RectPixels p)
    : rotation(r)
    , padding(p)
  {
  }
  std::array<double, 4> rotation{ 0.0,
                                  0.0,
                                  0.0,
                                  0.0 };           // Left, Right, Top, Bottom
  std::array<signed int, 4> padding{ 0, 0, 0, 0 }; // Left, Right, Top, Bottom
};

struct Profile
{
  std::string name = "(empty)";
  int profile_id = 0;
  bool use_default_padding = true;
  std::vector<UserDisplay> displays = { UserDisplay() };

  bool ValidateParameters() const;
};

class Settings
{
public:
  std::string active_profile_name = Profile().name; // use default profile name
  bool track_on_start = false;
  bool quit_on_loss_of_trackir = false;
  bool auto_retry = false;
  bool auto_find_track_ir_dll = true;
  std::string track_ir_dll_folder = "";
  bool pipe_server_enabled = false;
  std::string pipe_server_name = "trackirmouse";
  bool hotkey_enabled = false;
  std::string hotkey_name = "F18";
  spdlog::level::level_enum log_level = spdlog::level::info;

  std::array<Pixels, 4> default_padding = { 0, 0, 0, 0 };
  // invariant: the active profile member should always exist in the profiles
  // list
  std::vector<Profile> profiles = { Profile() };

private:
  std::string file_path_ = "";

public:
  Settings() {} // load with default values

  auto SaveToFile() -> void;

  // getter interface
  auto GetActiveProfile() -> Profile&;
  auto ApplyNecessaryDefaults() -> void;
  auto GetProfileNames() -> std::vector<std::string>;

  // setter interface
  auto SetActiveProfile(std::string profile_name) -> bool;
  auto SetActProfDisplayMappingParam(int display_number,
                                     int param_type,
                                     int param_side,
                                     double param) -> void;
  auto AddProfile(std::string new_profile_name) -> void;
  auto RemoveProfile(std::string profile_name) -> void;
  auto DuplicateActiveProfile() -> void;

  auto SetLogLevel(std::string level) -> void;
  auto GetLogLevelString() -> std::string;
};

auto
Get() -> std::shared_ptr<Settings>;

auto
GetCopy() -> Settings;

auto
Set(Settings c) -> void;

auto
SetToDefaults() -> void;

void
LoadFromFile(std::string filename);

using UserInput = std::vector<UserDisplay>;

bool
ValidateUserInput(const UserInput& displays);
} // namespace config

#endif // TIRMOUSE_CONFIG_HPP
