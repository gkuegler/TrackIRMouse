#ifndef TRACKIRMOUSE_CONFIG_HPP
#define TRACKIRMOUSE_CONFIG_HPP

#include <array>
#include <map>
#include <string>

#include "log.hpp"

#include "types.hpp"

namespace settings {

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

  int GetDisplayCount() const;
  bool ValidateParameters() const;
};

class Settings
{
public:
  // TODO: change config to settings
  // TODO: move user data to config class
  // UserData user_data;
  std::string active_profile_name = Profile().name; // use default profile name
  bool track_on_start = false;
  bool quit_on_loss_of_trackir = false;
  bool auto_find_track_ir_dll = true;
  std::string track_ir_dll_folder = "";
  bool pipe_server_enabled = false;
  std::string pipe_server_name = "";
  bool hotkey_enabled = false;
  spdlog::level::level_enum log_level = spdlog::level::info;

  std::array<Pixels, 4> default_padding = { 0, 0, 0, 0 };
  // invariant: the active profile member will always exist in the profiles
  // list
  std::vector<Profile> profiles = { Profile() };

private:
  std::string file_path_ = "";

public:
  Settings() {}                         // load with default values
  Settings(const std::string filename); // load from file

  void SaveToFile();

  // getter interface
  auto GetActiveProfile() -> Profile&;
  // TODO: moved to profile class
  auto GetActiveProfileDisplayCount() -> int;
  auto GetProfileNames() -> std::vector<std::string>;

  // setter interface
  bool SetActiveProfile(std::string profile_name);
  void SetActProfDisplayMappingParam(int display_number,
                                     int param_type,
                                     int param_side,
                                     double param);
  void AddProfile(std::string new_profile_name);
  void RemoveProfile(std::string profile_name);
  void DuplicateActiveProfile();
};

auto
Get() -> std::shared_ptr<Settings>;

auto
GetCopy() -> Settings;

auto
Set(Settings c) -> void;

// TODO: changed to std::optional or something
struct LoadResults
{
  bool success = false;
  std::string err_msg = "";
};
LoadResults
LoadFromFile(std::string filename);

using UserInput = std::vector<UserDisplay>;

bool
ValidateUserInput(const UserInput& displays);
} // namespace config

#endif /* TRACKIRMOUSE_CONFIG_HPP */
