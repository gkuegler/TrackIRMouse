#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

// TODO: I should revert this to use the name and not the title? No because if I can't load the
// title mapping file I lose the ability to coonect with NPTrackIR.
// TODO make a verify game titles button?

// JSON only supports map type of which key_type is convertible from std::string
// this is inherent to json file; example: 11220 = "Game Title". That is why I
// have to get my profile id as a string instead of an int.
using NpTitlesMap = std::map<std::string, std::string>;

// Used for Ui.
struct NpTitle
{
  std::string id;
  std::string name;
};

/*
 * Load game titles by ID from file.
 */
NpTitlesMap
GetTitleIdsFromFile();
/*
 * Sort title by name then number. Some valid title ID's are not named,
 * becuase I did not have enough time to find all of the names.
 */
using NpTitleList = std::vector<NpTitle>;
std::vector<NpTitle>
SortGameTitles(std::shared_ptr<NpTitlesMap> titles);
