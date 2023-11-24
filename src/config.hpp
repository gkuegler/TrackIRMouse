#ifndef TRACKIRMOUSE_CONFIG_HPP
#define TRACKIRMOUSE_CONFIG_HPP

#include <array>
#include <map>
#include <string>

#include "log.hpp"
// TODO: remove toml exception dependency
#include "toml/exception.hpp"
#include "types.hpp"

// TODO: move config validation to new file
// TODO:

namespace config {

struct Profile
{
  std::string name = "(empty)";
  int profile_id = 0;
  bool use_default_padding = true;
  std::vector<UserDisplay> displays = {};
};

// TODO: rename user preferences
// things which can be modified
struct UserData
{
  std::string active_profile_name = Profile().name; // use default profile name
  bool track_on_start = true;
  bool quit_on_loss_of_trackir = true;
  bool auto_find_track_ir_dll = true;
  std::string track_ir_dll_folder = "";
  bool pipe_server_enabled = true;
  std::string pipe_server_name = "";
  bool hotkey_enabled = true;
  spdlog::level::level_enum log_level = spdlog::level::info;

  std::array<Pixels, 4> default_padding = { 0, 0, 0, 0 };

  // invariant: the active profile member will always exist in the profiles
  // list
  std::vector<Profile> profiles = { Profile() };
};

class Config
{
public:
  UserData user_data;

private:
  std::string file_path_ = "";

public:
  Config() {}                         // load with default values
  Config(const std::string filename); // load from file

  void SaveToFile();

  // getter interface
  auto GetActiveProfile() -> Profile&;
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
Get() -> std::shared_ptr<Config>;

auto
Set(Config c) -> void;

} // namespace config

#endif /* TRACKIRMOUSE_CONFIG_HPP */
