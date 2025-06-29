#pragma once

#include <array>
#include <filesystem>
#include <map>
#include <string>

#include "log.hpp"
#include "types.hpp"

// TODO: Move global to types?
struct DisplayParameters
{
  RectDegrees rotation;
  RectPixels padding;
};

struct Profile
{
  std::string name = "(empty)";
  int profile_id = 0;
  bool use_default_padding = true;
  std::vector<DisplayParameters> displays = { DisplayParameters() };

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

  RectPixels default_padding = { 0, 0, 0, 0 };
  // invariant: the active profile member should always exist in the profiles
  // list
  std::vector<Profile> profiles = { Profile() };

public:
  Settings() = default; // create with default values

  auto SaveToFile() -> void;
  static auto LoadFromFile() -> Settings;

  // getter interface
  auto GetActiveProfileRef() -> Profile&;
  auto ApplyDefaultPaddingToAllDisplays() -> void;
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