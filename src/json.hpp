#ifndef TRACKIRMOUSE_JSON_HPP
#define TRACKIRMOUSE_JSON_HPP

#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>

using json = nlohmann::json;

template<typename T>
auto
LoadJsonFromFileIntoObject(std::string filename) -> T
{
  std::ifstream f(filename);

  if (!f.good()) {
    auto message = "can't access file '" + filename + "'.";
    throw std::exception(message.c_str());
  }

  json j = json::parse(f);
  return j.template get<T>();
}

#endif /* TRACKIRMOUSE_JSON_HPP */
