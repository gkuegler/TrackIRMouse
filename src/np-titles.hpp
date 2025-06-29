#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

// TODO: I should revert this to use the name and not the title?
// TODO make a verify game titles button?

// JSON only supports map type of which key_type is convertible from std::string
// this is inherent to json file; example: 11220 = "Game Title". That is why I
// have to get my profile id as a string instead of an int.
using game_title_map_t = std::map<std::string, std::string>;

// Used for Ui.
struct GameTitle
{
  std::string name;
  std::string id;
};

using GameTitleList = std::vector<GameTitle>;

/*
 * Load game titles by ID from file.
 */
game_title_map_t
GetTitleIdsFromFile();
/*
 * Sort title by name then number. Some valid title ID's are not named,
 * becuase I did not have enough time to find all of the names.
 */
GameTitleList
SortGameTitles(std::shared_ptr<game_title_map_t> titles);
