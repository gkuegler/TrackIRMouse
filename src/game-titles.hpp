#pragma once
#ifndef TIRMOUSE_GAME_TITLES_HPP
#define TIRMOUSE_GAME_TITLES_HPP

#include <map>
#include <string>

// json only supports map type of which key_type is convertible from
// std::string this is inherent to json file; example: 11220 = "Game Title"
// that is why I have to get my profile id as a string instead of
// native int
using game_title_map_t = std::map<std::string, std::string>;

game_title_map_t
GetTitleIds();
#endif /* TIRMOUSE_GAME_TITLES_HPP */
