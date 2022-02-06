#ifndef TRACKIRMOUSE_CONFIG_H
#define TRACKIRMOUSE_CONFIG_H

#include "Constants.h"
#include "Log.hpp"

#define TOML11_PRESERVE_COMMENTS_BY_DEFAULT
#include <string>

#include "toml.hpp"

static constexpr std::array<std::string_view, 4> kBoundNames = {
    "left", "right", "top", "bottom"};

class CBounds {
 public:
  // Left, Right, Top, Bottom
  std::array<float, 4> rotationBounds{0.0, 0.0, 0.0, 0.0};
  std::array<int, 4> paddingBounds{0, 0, 0, 0};

  CBounds(){};
  CBounds(std::array<float, 4> &&rotations, std::array<int, 4> &&padding) {
    rotationBounds = rotations;
    paddingBounds = padding;
  }
};

struct SProfile {
  std::string name = "profile";
  int profile_ID = 0;
  bool useDefaultPadding = true;
  std::vector<CBounds> bounds;
};

struct SData {
  bool trackOnStart = true;
  bool quitOnLossOfTrackIr = true;
  bool watchdogEnabled = true;
  std::string trackIrDllFolder = "default";
  std::string activeProfileName = "Lorem Ipsum";

  std::vector<int> defaultPaddings = {0, 0, 0, 0};
  std::vector<SProfile> profiles;
};

class CConfig {
 public:
  int m_monitorCount = 0;
  std::string m_trackIrDllPath;

  SData data;

  CConfig(){};

  // Initializations Functions
  void ParseFile(const std::string);
  void LoadSettings();
  void SaveSettings();

  // Getter functions
  SProfile &GetActiveProfile();
  std::vector<std::string> GetProfileNames();
  int GetActiveProfileDisplayCount();

  void SetDisplayMappingParameter(int displayNumber, int parameterType,
                                  int parameterSide, float parameter);

  void AddProfile(std::string newProfileName);
  void RemoveProfile(std::string profileName);
  void DuplicateActiveProfile();

  void ClearData(){
    m_vData = toml::value();
    data = SData();
  }

 private:
  // toml data is not used after it's loaded
  toml::value m_vData;  // holds main toml object

  void LogTomlError(const std::exception &ex);
};

CConfig *GetGlobalConfig();
CConfig GetGlobalConfigCopy();
void ClearGlobalData();

extern CConfig g_config;

#endif /* TRACKIRMOUSE_CONFIG_H */
