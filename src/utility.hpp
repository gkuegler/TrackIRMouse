#ifndef TRACKIRMOUSE_UTIL_H
#define TRACKIRMOUSE_UTIL_H

#include <spdlog/spdlog.h>
#include <wx/arrstr.h>

#include <filesystem>
#include <string>
#include <vector>

namespace utility {
// template <typename C<T>> requires ??

// container must be an irritable array of type T,
// where type T can be implicitly converted to wxString
template<typename T>
wxArrayString
BuildWxArrayString(const T container)
{
  wxArrayString array;
  for (const auto& text : container) {
    array.Add(wxString(text), 1);
  }
  return array;
}

inline std::string
GetExecutableFolder()
{
  // TODO: use unicode to avoid maximum path length
  char full_path[MAX_PATH];
  auto n_char = GetModuleFileNameA(nullptr, full_path, MAX_PATH);
  if (n_char != 0 && n_char != MAX_PATH) {
    std::filesystem::path path(full_path);
    auto file_path = path.parent_path().string();
    SPDLOG_DEBUG(full_path);
    SPDLOG_DEBUG(file_path);
    return file_path;
  } else {
    auto last_error = GetLastError();
    if (ERROR_INSUFFICIENT_BUFFER == last_error) {
      throw std::runtime_error(
        std::format("GetModuleFileNameA failed. File path of executable was "
                    "too long. Consider moving executable and related folder "
                    "to a folder with a shorter path."));
    } else {
      throw std::runtime_error(std::format(
        "GetModuleFileNameA failed for unknown reason. GLE={}", last_error));
    }
  }
}
} // namespace util
#endif /* TRACKIRMOUSE_UTIL_H */
