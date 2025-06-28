#pragma once

#include <optional>
#include <shellapi.h>
#include <string>

#include "log.hpp"
#include "windows-wrapper.hpp"

void
ExecuteShellCommand(HWND hWnd,
                    std::string verb,
                    std::string file_path,
                    std::string parameters = "",
                    std::string directory = "")
{
  HINSTANCE result =
    ShellExecuteA(hWnd,
                  verb.c_str(),
                  file_path.c_str(),
                  parameters.empty() ? nullptr : parameters.c_str(), // lpParameters,
                  directory.empty() ? nullptr : directory.c_str(),   // lpDirectory,
                  SW_SHOWNORMAL);

  // A Microsoft backward compatibility thing.
  // INT_PTR: an integer guaranteed to be the length of the pointer.
  INT_PTR real_result = reinterpret_cast<INT_PTR>(result);

  if (real_result > 32) {
    spdlog::debug("The shell execute operation succeeded.");
    return;
  }

  switch (real_result) {
    case 0:
      throw std::runtime_error("The operating system is out of memory.");
      break;
    case ERROR_FILE_NOT_FOUND:
      throw std::runtime_error("The system cannot find the file specified.");
      break;
    case ERROR_PATH_NOT_FOUND:
      throw std::runtime_error("ERROR_BAD_FORMAT");
      break;
    case SE_ERR_ACCESSDENIED:
      throw std::runtime_error("SE_ERR_ACCESSDENIED");
      break;
    case SE_ERR_ASSOCINCOMPLETE:
      throw std::runtime_error("SE_ERR_ASSOCINCOMPLETE");
      break;
    case SE_ERR_DDEBUSY:
      throw std::runtime_error("SE_ERR_DDEBUSY");
      break;
    case SE_ERR_DDEFAIL:
      throw std::runtime_error("SE_ERR_DDEFAIL");
      break;
    case SE_ERR_DDETIMEOUT:
      throw std::runtime_error("SE_ERR_DDETIMEOUT");
      break;
    case SE_ERR_DLLNOTFOUND:
      throw std::runtime_error("SE_ERR_DLLNOTFOUND");
      break;
    // The SE_ERR_NOASSOC error is returned when there is no default program
    // for the file extension. In modern Windows, when the verb "open" is
    // used, a dialogue is automatically presented to the user to specify the
    // desired program to open the file with.
    case SE_ERR_NOASSOC:
      throw std::runtime_error(
        std::format("No associated program for the file '{}' was found.", file_path));
      break;
    case SE_ERR_OOM:
      throw std::runtime_error("SE_ERR_OOM");
      break;
    case SE_ERR_SHARE:
      throw std::runtime_error("SE_ERR_SHARE");
      break;
    default:
      throw std::runtime_error(
        std::format("Unknown error executing shell command. GLE={}", GetLastError()));
      break;
  }
}
