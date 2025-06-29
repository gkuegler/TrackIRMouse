// export module GameTitles;

#include "np-titles.hpp"

#include "json.hpp"
#include "utility.hpp"

#include <format>
#include <fstream>
#include <spdlog/spdlog.h>

static const game_title_map_t default_game_titles{
  { "1001", "IL-2 Forgotten Battles" },
  { "1002", "Lock-On Modern Air" },
  { "1003", "Black Shark" },
  { "1004", "Tom Clancy's H.A.W.X." },
  { "1005", "LockOn: Flaming Cliffs 2" },
  { "1006", "DCS: A-10C" },
  { "1007", "Tom Clancy's H.A.W.X." },
  { "1008", "IL-2 Struremovik: Battle" },
  { "1009", "The Crew" },
  { "1025", "Down In Flames" },
};

game_title_map_t
GetTitleIdsFromFile()
{
  constexpr const char* filename = "trackir-game-title-ids.json";
  constexpr const char* instructions = "Couldn't load game titles from local file '{}'.\n"
                                       "A short default title list shall be loaded.\n\nReason:\n{}";

  try {
    return LoadJsonFromFileIntoObject<game_title_map_t>(
      utility::GetAbsolutePathRelativeToExeFolder(filename));
  } catch (const std::exception& ex) {
    // TODO: make a standard exception format?
    spdlog::error(instructions, filename, ex.what());
  }

  return default_game_titles;
}

GameTitleList
SortGameTitles(std::shared_ptr<game_title_map_t> titles)
{
  GameTitleList named;
  GameTitleList unnamed;
  named.reserve(titles->size());
  unnamed.reserve(titles->size());

  for (auto& [key, item] : *titles) {
    if (item.empty()) {
      unnamed.push_back({ item, key });
    } else {
      named.push_back({ item, key });
    }
  }

  std::sort(named.begin(), named.end(), [](const GameTitle& left, const GameTitle& right) {
    return left.name < right.name;
  });

  std::sort(unnamed.begin(), unnamed.end(), [](const GameTitle& left, const GameTitle& right) {
    return std::stoi(left.id) < std::stoi(right.id);
  });

  named.insert(named.end(), unnamed.begin(), unnamed.end());

  return named;
}