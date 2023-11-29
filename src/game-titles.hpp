#ifndef TRACKIRMOUSE_GAME_TITLES_HPP
#define TRACKIRMOUSE_GAME_TITLES_HPP

#include <map>
#include <string>

// toml::get only supports map type of which key_type is convertible from
// std::string this is inherent to toml file; example: 11220 = "Game Title"
// that is why I have to get my profile id as a string instead of
// native int
using game_title_map_t = std::map<std::string, std::string>;

game_title_map_t
GetTitleIds();
#endif /* TRACKIRMOUSE_GAME_TITLES_HPP */
