#ifndef TRACKIRMOUSE_CONFIG_HPP
#define TRACKIRMOUSE_CONFIG_HPP

#include <array>
#include <map>
#include <string>

#include "log.hpp"
#include "toml/exception.hpp"
#include "types.hpp"

const constexpr std::array<std::string_view, 4> kBoundNames = {"left", "right",
                                                               "top", "bottom"};

namespace config {

using bounds_t = std::array<deg, 4>;
using pad_t = std::array<pixels, 4>;

struct Display {
  Display(bounds_t r, pad_t p) : rotation(r), padding(p) {}
  bounds_t rotation{0.0, 0.0, 0.0, 0.0};  // Left, Right, Top, Bottom
  pad_t padding{0, 0, 0, 0};              // Left, Right, Top, Bottom
};

struct Profile {
  std::string name = "profile";
  int profileId = 0;
  bool useDefaultPadding = true;
  std::vector<Display> displays;
};

struct UserData {
  bool trackOnStart = true;
  bool quitOnLossOfTrackIr = true;
  bool watchdogEnabled = true;
  int logLevel = spdlog::level::trace;
  int logLevelFile = spdlog::level::trace;
  int logLevelTextControl = spdlog::level::info;
  std::string trackIrDllFolder = "default";
  std::string activeProfileName = "Lorem Ipsum";
  std::array<pixels, 4> defaultPaddings = {0, 0, 0, 0};
  std::vector<Profile> profiles;
};

struct EnvData {
  int monitorCount = 0;
  std::string trackIrDllPath = "";
};

// Initializations Functions
void InitializeSettingsSingletonsFromFile(const std::string);
void WriteSettingsToFile();

// Getter interfaces
UserData GetUserData();
UserData &GetUserDataMutable();
EnvData GetEnvironmentData();
EnvData &GetEnvironmentDataMutable();
Profile GetActiveProfile();
Profile &GetActiveProfileMutable();
int GetActiveProfileDisplayCount();
std::vector<std::string> GetProfileNames();

// Setter interfaces
void SetLogLevel(spdlog::level::level_enum level);
bool SetActiveProfile(std::string profileName);
void SetActProfDisplayMappingParam(int displayNumber, int parameterType,
                                   int parameterSide, double parameter);
void AddProfile(std::string newProfileName);
void RemoveProfile(std::string profileName);
void DuplicateActiveProfile();

using UserInput = std::vector<Display>;
bool ValidateUserInput(const UserInput &displays);

// toml::get only supports map type of which key_type is convertible from
// std::string this is inherent to toml file; example: 11220 = "Game Title"
// that is why I have to get my profile id as a string instead of native int
using game_title_map_t = std::map<const std::string, std::string>;
game_title_map_t GetTitleIds();
}  // namespace config

#endif /* TRACKIRMOUSE_CONFIG_HPP */
