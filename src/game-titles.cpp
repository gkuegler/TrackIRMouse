// export module GameTitles;

#include "game-titles.hpp"

#include "json.hpp"
#include "utility.hpp"

#include <format>
#include <fstream>
#include <spdlog/spdlog.h>

// load game titles by ID number from file
game_title_map_t
GetTitleIds()
{
  // loading from file allows the game titles to be modified for future
  // natural point continually adds new titles
  constexpr auto filename = "track-ir-numbers.json";
  const std::string instructions =
    "Couldn't load game titles from resource file.\n"
    "A courtesy sub-sample of the title list will provided from source code.";

  try {
    // load, parse, and return as std::map
    return LoadJsonFromFileIntoObject<game_title_map_t>(
      utility::GetAbsolutePathRelativeToExeFolder(filename));
  } catch (const std::exception& ex) {
    spdlog::error("Error opening game titles file '{}'. A sample file will be "
                  "loaded.\n\n{}",
                  filename,
                  ex.what());
  }

  // provide sample list since full list couldn't be loaded from file
  game_title_map_t sample_map;
  sample_map["1001"] = "IL-2 Forgotten Battles";
  sample_map["1002"] = "Lock-On Modern Air";
  sample_map["1003"] = "Black Shark";
  sample_map["1004"] = "Tom Clancy's H.A.W.X.";
  sample_map["1005"] = "LockOn: Flaming Cliffs 2";
  sample_map["1006"] = "DCS: A-10C";
  sample_map["1007"] = "Tom Clancy's H.A.W.X.";
  sample_map["1008"] = "IL-2 Struremovik: Battle";
  sample_map["1009"] = "The Crew";
  sample_map["1025"] = "Down In Flames";
  return sample_map;
}
