#ifndef TRACKIRMOUSE_CONFIG_HPP
#define TRACKIRMOUSE_CONFIG_HPP

#include <array>
#include <string>

#include "constants.hpp"
#include "log.hpp"

// i may need static keyword here
const constexpr std::array<std::string_view, 4> kBoundNames = {"left", "right",
                                                               "top", "bottom"};

namespace config {

using t_bounds = std::array<double, 4>;
using t_pad = std::array<int, 4>;

struct Display {
  Display(t_bounds r, t_pad p) : rotation(r), padding(p) {}
  t_bounds rotation{0.0, 0.0, 0.0, 0.0};  // Left, Right, Top, Bottom
  t_pad padding{0, 0, 0, 0};              // Left, Right, Top, Bottom
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
  std::string trackIrDllFolder = "default";
  std::string activeProfileName = "Lorem Ipsum";
  std::array<int, 4> defaultPaddings = {0, 0, 0, 0};
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

bool SetActiveProfile(std::string profileName);
void SetActProfDisplayMappingParam(int displayNumber, int parameterType,
                                   int parameterSide, double parameter);

void AddProfile(std::string newProfileName);
void RemoveProfile(std::string profileName);
void DuplicateActiveProfile();

using UserInput = std::vector<Display>;
bool ValidateUserInput(const UserInput &displays);

}  // namespace config

#endif /* TRACKIRMOUSE_CONFIG_HPP */
