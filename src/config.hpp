#ifndef TRACKIRMOUSE_CONFIG_HPP
#define TRACKIRMOUSE_CONFIG_HPP

#include <array>
#include <string>

#include "log.hpp"
#include "toml/exception.hpp"
#include "types.hpp"

// i may need static keyword here
const constexpr std::array<std::string_view, 4> kBoundNames = {"left", "right",
                                                               "top", "bottom"};

namespace config {

using bounds_t = std::array<deg, 4>;
using pad_t = std::array<pixles, 4>;

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
void LoadSettingsFromFile(const std::string);
void WriteSettingsToFile();

UserData GetUserData();
UserData &GetUserDataMutable();

EnvData GetEnvironmentData();
EnvData &GetEnvironmentDataMutable();

// Getter functions
Profile GetActiveProfile();
Profile &GetActiveProfileMutable();
int GetActiveProfileDisplayCount();

std::vector<std::string> GetProfileNames();

// Setter Functions
bool SetActiveProfile(std::string profileName);
void SetActProfDisplayMappingParam(int displayNumber, int parameterType,
                                   int parameterSide, double parameter);
void SetLogLevel(spdlog::level::level_enum level);
void AddProfile(std::string newProfileName);
void RemoveProfile(std::string profileName);
void DuplicateActiveProfile();

using UserInput = std::vector<Display>;
bool ValidateUserInput(const UserInput &displays);

}  // namespace config

#endif /* TRACKIRMOUSE_CONFIG_HPP */
