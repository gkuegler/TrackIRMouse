// export module GameTitles;

#include "game-titles.hpp"

#include "util.hpp"

#define TOML11_PRESERVE_COMMENTS_BY_DEFAULT
#include "toml.hpp"
#include <format>

// toml::get only supports map type of which key_type is convertible from
// std::string this is inherent to toml file; example: 11220 = "Game Title"
// that is why I have to get my profile id as a string instead of
// native int
using game_title_map_t = std::map<const std::string, std::string>;

// load game titles by ID number from file
game_title_map_t
GetTitleIds()
{
  // loading from file allows the game titles to be modified for future
  // natural point continually adds new titles
  constexpr auto filename = "track-ir-numbers.toml";
  const std::string instructions =
    "Couldn't load game titles from resource file.\n"
    "A courtesy sub-sample of the title list will provided from source code.";
  std::string err_msg = "lorem ipsum";

  auto full_path = util::GetExecutableFolder() + "\\" + filename;

  try {
    // load, parse, and return as map
    auto data = toml::parse(full_path);
    return toml::find<game_title_map_t>(data, "data");
  } catch (const toml::syntax_error& ex) {
    err_msg = std::format(
      "Syntax error in toml file: \"{}\"\nSee error message below for hints "
      "on how to fix.\n{}",
      full_path,
      ex.what());
  } catch (const toml::type_error& ex) {
    err_msg = std::format("Incorrect type when parsing toml file \"{}\".\n\n{}",
                          full_path,
                          ex.what());
  } catch (const std::out_of_range& ex) {
    err_msg = std::format(
      "Missing data in toml file \"{}\".\n\n{}", full_path, ex.what());
  } catch (std::runtime_error& ex) {
    err_msg = std::format("Couldn't Find or Open \"{}\"", full_path);
  } catch (...) {
    err_msg = std::format(
      "exception has gone unhandled loading \"{}\" and verifying values.",
      full_path);
  }

  spdlog::error("{}\n\n{}", err_msg, instructions);

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
