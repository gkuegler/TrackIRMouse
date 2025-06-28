#pragma once

#include <map>
#include <string>

// JSON only supports map type of which key_type is convertible from std::string
// this is inherent to json file; example: 11220 = "Game Title". That is why I
// have to get my profile id as a string instead of an int.
using game_title_map_t = std::map<std::string, std::string>;

/*
 * Load game titles by ID from file.
 */
game_title_map_t
GetTitleIds();
