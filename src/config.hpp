#ifndef TRACKIRMOUSE_CONFIG_HPP
#define TRACKIRMOUSE_CONFIG_HPP

#include <array>
#include <map>
#include <string>

#include "log.hpp"
// TODO: remove toml exception dependency
#include "toml11/toml/exception.hpp"
#include "types.hpp"

const constexpr std::array<std::string_view, 4> kBoundNames = {"left", "right",
                                                               "top", "bottom"};
namespace config {

struct Display {
  Display(RectDegrees r, RectPixels p) : rotation(r), padding(p) {}
  RectDegrees rotation{0.0, 0.0, 0.0, 0.0};  // Left, Right, Top, Bottom
  RectPixels padding{0, 0, 0, 0};            // Left, Right, Top, Bottom
};

struct Profile {
  std::string name = "(empty)";
  int profileId = 0;
  bool useDefaultPadding = true;
  std::vector<Display> displays = {};
};
// const Profile kDefaultProfile = {"empty", 0, true, {}};

struct UserData {
  bool trackOnStart = true;
  bool quitOnLossOfTrackIr = true;
  bool watchdogEnabled = true;
  std::string pipeServerName = "";
  spdlog::level::level_enum logLevel = spdlog::level::info;
  bool autoFindTrackIrDll = true;
  std::string trackIrDllFolder = "";
  std::string activeProfileName = Profile().name;
  std::array<Pixels, 4> defaultPaddings = {0, 0, 0, 0};
  std::vector<Profile> profiles = {Profile()};
};

struct EnvData {
  int monitorCount = 0;
  std::string trackIrDllPath = "";
};

class Config {
 public:
  UserData userData;
  EnvData envData;

  Config() {}                          // load with default values
  Config(const std::string filename);  // load from file

  void SaveToFile(std::string filename);
  // void SaveToFile();

  // getter interface
  Profile &GetActiveProfile();
  int GetActiveProfileDisplayCount();
  std::vector<std::string> GetProfileNames();

  // setter interface
  bool SetActiveProfile(std::string profileName);
  void SetActProfDisplayMappingParam(int displayNumber, int parameterType,
                                     int parameterSide, double parameter);
  void AddProfile(std::string newProfileName);
  void RemoveProfile(std::string profileName);
  void DuplicateActiveProfile();

 private:
  std::string filename_ = "";
};

struct ConfigReturn {
  retcode code = retcode::fail;
  std::string err_msg = "";
  Config config;
};

ConfigReturn LoadFromFile(std::string filename);
std::shared_ptr<Config> Get();
void Set(const Config c);

using UserInput = std::vector<Display>;
bool ValidateUserInput(const UserInput &displays);

// toml::get only supports map type of which key_type is convertible from
// std::string this is inherent to toml file; example: 11220 = "Game Title"
// that is why I have to get my profile id as a string instead of native int
using game_title_map_t = std::map<const std::string, std::string>;
game_title_map_t GetTitleIds();
}  // namespace config

#endif /* TRACKIRMOUSE_CONFIG_HPP */
