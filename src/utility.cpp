#pragma once

#include "utility.hpp"

#include <spdlog/spdlog.h>

#include <array>
#include <filesystem>

#include "windows-wrapper.hpp"

#define STR_IMPL_(x) #x     // stringify arg
#define STR(x) STR_IMPL_(x) // indirection to expand

#define STR_IMPL_VA(...) #__VA_ARGS__       // stringify arg
#define STRVA(...) STR_IMPL_VA(__VA_ARGS__) // indirection to expand arg macros

namespace utility {

std::string
GetAbsolutePathRelativeToExeFolder(std::string relative_path)
{
  constexpr size_t WIN_LONG_PATH_LIMIT = 32767;

  std::array<char, WIN_LONG_PATH_LIMIT> mod_path{}; // init to 0's

  // Note: the return value is the length of the string that is copied to the
  // buffer, in characters, not including the terminating null character.
  auto n_char = GetModuleFileNameA(nullptr, &mod_path[0], mod_path.size());

  if (n_char != 0 && n_char < MAX_PATH) {
    auto folder = std::filesystem::path(&mod_path[0]).parent_path();
    return (folder.append(relative_path)).string();
  }

  auto err = GetLastError();
  if (ERROR_INSUFFICIENT_BUFFER == err) {
    throw std::runtime_error("GetModuleFileNameA failed. File path of executable was "
                             "too long. Consider moving executable and related folder "
                             "to a folder with a shorter path.");
  } else {
    throw std::runtime_error(
      std::format("GetModuleFileNameA failed for unknown reason. GLE={}", err));
  }
}

} // namespace utility
