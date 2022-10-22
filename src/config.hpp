#ifndef TRACKIRMOUSE_CONFIG_HPP
#define TRACKIRMOUSE_CONFIG_HPP

#include <array>
#include <map>
#include <string>

#include "log.hpp"
// TODO: remove toml exception dependency
#include "toml11/toml/exception.hpp"
#include "types.hpp"

namespace config {

struct UserDisplay
{
  UserDisplay(RectDegrees r, RectPixels p)
    : rotation(r)
    , padding(p)
  {
  }
  RectDegrees rotation{ 0.0, 0.0, 0.0, 0.0 }; // Left, Right, Top, Bottom
  RectPixels padding{ 0, 0, 0, 0 };           // Left, Right, Top, Bottom
};

using UserInput = std::vector<UserDisplay>;

struct Profile
{
  std::string name = "(empty)";
  int profile_id = 0;
  bool use_default_padding = true;
  std::vector<UserDisplay> displays = {};
};
// const Profile kDefaultProfile = {"empty", 0, true, {}};

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

struct EnvData
{
  int monitor_count = 0;
  std::string track_ir_dll_path = "";
};

class Config
{
public:
  UserData user_data;
  EnvData env_data;

  Config() {}                         // load with default values
  Config(const std::string filename); // load from file

  void save_to_file(std::string filename);
  // void save_to_file();

  // getter interface
  Profile& GetActiveProfile();
  int GetActiveProfileDisplayCount();
  std::vector<std::string> GetProfileNames();

  // setter interface
  bool SetActiveProfile(std::string profile_name);
  void SetActProfDisplayMappingParam(int display_number,
                                     int param_type,
                                     int param_side,
                                     double param);
  void AddProfile(std::string new_profile_name);
  void RemoveProfile(std::string profile_name);
  void DuplicateActiveProfile();

private:
  std::string filename_ = "";
};

struct ConfigReturn
{
  retcode code = retcode::fail;
  std::string err_msg = "";
  Config config;
};

ConfigReturn
LoadFromFile(std::string filename);
std::shared_ptr<Config>
Get();
void
Set(const Config c);
bool
ValidateUserInput(const UserInput& displays);

// toml::get only supports map type of which key_type is convertible from
// std::string this is inherent to toml file; example: 11220 = "Game Title"
// that is why I have to get my profile id as a string instead of
// native int
using game_title_map_t = std::map<const std::string, std::string>;

game_title_map_t
GetTitleIds();
} // namespace config

#endif /* TRACKIRMOUSE_CONFIG_HPP */
