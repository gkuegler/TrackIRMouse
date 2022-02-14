#ifndef TRACKIRMOUSE_CONFIG_H
#define TRACKIRMOUSE_CONFIG_H

#include "constants.hpp"
#include "log.hpp"

#define TOML11_PRESERVE_COMMENTS_BY_DEFAULT
#include <string>

#include "toml.hpp"

// TODO: make a config namespace
static constexpr std::array<std::string_view, 4> kBoundNames = {
    "left", "right", "top", "bottom"};


namespace config {

struct Display {
  std::array<double, 4> rotation{0.0, 0.0, 0.0, 0.0};  // Left, Right, Top, Bottom
  std::array<int, 4> padding{0, 0, 0, 0};  // Left, Right, Top, Bottom
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

  std::array<int> defaultPaddings = {0, 0, 0, 0};
  std::vector<SProfile> profiles;
};

struct EnvData {
  int m_monitorCount = 0;
  std::string m_trackIrDllPath;
}

class CConfig {
 public:
  UserData userData;
  EnvData environmentData;

  CConfig(){};

  // Initializations Functions
  void ParseFile(const std::string);
  void LoadSettings();
  void SaveSettings();

  // Getter functions
  Profile &GetActiveProfile();
  const Profile &GetActiveProfile();

  std::vector<std::string> GetProfileNames();
  int GetActiveProfileDisplayCount();

  UserData GetUserData():
  UserData& GetUserData():

  bool SetActiveProfile(std::string profileName);
  void SetDisplayMappingParameter(int displayNumber, int parameterType,
                                  int parameterSide, double parameter);

  void AddProfile(std::string newProfileName);
  void RemoveProfile(std::string profileName);
  void DuplicateActiveProfile();

  void ClearData() {
    m_vData = toml::value();
    data = SData();
  }

 private:
  // toml data is not used after it's loaded
  toml::value m_vData;  // holds main toml object
};

CConfig *GetGlobalConfig();
CConfig GetGlobalConfigCopy();
void ClearGlobalData();

extern CConfig g_config;
} // namespace config

using UserInput = std::vector<Display>;
bool ValidateUserInput(const UserInput &displays);

#endif /* TRACKIRMOUSE_CONFIG_H */
