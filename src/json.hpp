#pragma once
#ifndef TIRMOUSE_JSON_HPP
#define TIRMOUSE_JSON_HPP

#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>

using json = nlohmann::json;

template<typename T>
auto
LoadJsonFromFileIntoObject(std::string filename) -> T
{
  std::ifstream f(filename);

  if (!f.good()) {
    throw std::runtime_error("can't access file '" + filename + "'.");
  }

  json j = json::parse(f);
  return j.template get<T>();
}

#endif /* TIRMOUSE_JSON_HPP */
