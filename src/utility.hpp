#pragma once

#include <spdlog/spdlog.h>


#include <filesystem>
#include <string>
#include <vector>

#define STR_IMPL_(x) #x     // stringify arg
#define STR(x) STR_IMPL_(x) // indirection to expand

#define STR_IMPL_VA(...) #__VA_ARGS__       // stringify arg
#define STRVA(...) STR_IMPL_VA(__VA_ARGS__) // indirection to expand arg macros

namespace utility {



inline std::wstring
Utf8ToWideString(std ::string utf8text)
{
  const int buffsize = utf8text.length() * 3;
  std::vector<WCHAR> buff(buffsize);
  int result =
    MultiByteToWideChar(CP_UTF8,
                        // MB_ERR_INVALID_CHARS, // I feel like this should be
                        // the smart choice, but this causes an error.
                        MB_COMPOSITE,
                        utf8text.c_str(),
                        utf8text.length(),
                        buff.data(),
                        buffsize);

  if (0 == result) {
    throw std::runtime_error(
      std::format("Windows Error: failed to convert 'string' to 'wstring' "
                  "with error code: {}",
                  GetLastError()));
  }

  return std::wstring(buff.data());
}

inline std::string
WideToUtf8String(std::wstring wtext)
{
  const int buffsize = wtext.length();
  std::vector<char> buff(buffsize);
  BOOL used_default_char = 0;
  int result =
    WideCharToMultiByte(CP_UTF8,
                        // MB_ERR_INVALID_CHARS, // I feel like this should be
                        // the smart choice, but this causes an error.
                        MB_COMPOSITE,
                        wtext.c_str(),
                        wtext.length(),
                        buff.data(),
                        buffsize,
                        NULL, // default character if can't be represented
                        &used_default_char // flag if default character was used
    );

  if (0 == result || used_default_char) {
    throw std::runtime_error(
      std::format("Windows Error: failed to convert 'wstring' to 'string' "
                  "with error code: {}",
                  GetLastError()));
  }

  return std::string(buff.data());
}

inline std::string
GetExecutableFolder()
{
  // TODO: use unicode for users with a different multilingual plane
  // TODO: avoid maximum path length, see ExecuteShellCommand
  char full_path[MAX_PATH];
  auto n_char = GetModuleFileNameA(nullptr, full_path, MAX_PATH);
  if (n_char != 0 && n_char != MAX_PATH) {
    std::filesystem::path path(full_path);
    std::string folder_path = path.parent_path().string();
    SPDLOG_DEBUG(full_path);
    SPDLOG_DEBUG(folder_path);
    return folder_path;
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

inline std::string
GetAbsolutePathBasedFromExeFolder(std::string base_path)
{
  // TODO: use unicode?
  // TODO: support long paths?
  // Windows long path support prefix.
  // https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=registry
  // static const std::string LONG_PATH_PREFIX = "\\\\\?\\";
  // constexpr const int MAX_LONG_PATH = 32767;
  // std::array<char, MAX_LONG_PATH> exe;

  char exe[MAX_PATH];
  auto n_char = GetModuleFileNameA(nullptr, exe, MAX_PATH);

  if (n_char != 0 && n_char != MAX_PATH) {
    auto abs_path = std::filesystem::path(exe).parent_path() / base_path;
    return abs_path.string();
  } else {
    auto last_error = GetLastError();
    if (ERROR_INSUFFICIENT_BUFFER == last_error) {
      throw std::runtime_error(
        "GetModuleFileNameA failed. File path of executable was "
        "too long. Consider moving executable and related folder "
        "to a folder with a shorter path.");
    } else {
      throw std::runtime_error(std::format(
        "GetModuleFileNameA failed for unknown reason. GLE={}", last_error));
    }
  }
}

inline std::string
GetAbsolutePathBasedFromExeFolderUnicode(std::string base_path)
{
  // TODO: use unicode?
  // TODO: support long paths?
  // Windows long path support prefix.
  // https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=registry
  // static const std::string LONG_PATH_PREFIX = "\\\\\?\\";
  // constexpr const int MAX_LONG_PATH = 32767;
  // std::array<char, MAX_LONG_PATH> exe;

  WCHAR exe[MAX_PATH];
  auto n_char = GetModuleFileNameW(nullptr, exe, MAX_PATH);

  if (n_char != 0 && n_char != MAX_PATH) {
    // auto abs_path = LONG_PATH_PREFIX + std::string(exe) + "\\" + base_path;
    auto base = WideToUtf8String(exe);
    auto exe_path = std::filesystem::path();
    auto abs_path = exe_path.parent_path();
    abs_path += base_path;

    // SPDLOG_DEBUG(exe);
    // SPDLOG_DEBUG(abs_path.string());
    return abs_path.string();
  } else {
    auto last_error = GetLastError();
    if (ERROR_INSUFFICIENT_BUFFER == last_error) {
      throw std::runtime_error(
        "GetModuleFileNameA failed. File path of executable was "
        "too long. Consider moving executable and related folder "
        "to a folder with a shorter path.");
    } else {
      throw std::runtime_error(std::format(
        "GetModuleFileNameA failed for unknown reason. GLE={}", last_error));
    }
  }
}
} // namespace util