#ifndef TRACKIRMOUSE_REGISTRY_HPP
#define TRACKIRMOUSE_REGISTRY_HPP

#include <windows.h>

#include <string>

#include "log.hpp"

typedef struct RegistryQuery_
{
  int result;
  std::string result_string;
  std::string value;
} RegistryQuery;

/**
 * [GetStringFromRegistry description]
 * @param  parent_key [description]
 * @param  sub_key     [description]
 * @param  sub_value   [description]
 * @return            [description]
 */
RegistryQuery
GetStringFromRegistry(HKEY parent_key,
                      const char* sub_key,
                      const char* sub_value)
{
  //////////////////////////////////////////////////////////////////////
  //                         Opening The Key                          //
  //////////////////////////////////////////////////////////////////////

  HKEY hKey = 0;

  LSTATUS status_key_open =
    RegOpenKeyExA(parent_key, // should usually be HKEY_CURRENT_USER
                  sub_key,
                  0,        //[in]           DWORD  ulOptions,
                  KEY_READ, //[in]           REGSAM samDesired,
                  &hKey);

  if (ERROR_FILE_NOT_FOUND == status_key_open) {
    return RegistryQuery{ ERROR_FILE_NOT_FOUND, "Registry key not found.", "" };
  }

  // Catch all other errors
  if (ERROR_SUCCESS != status_key_open) {
    return RegistryQuery{ status_key_open, "Could not open registry key.", "" };
  }

  //////////////////////////////////////////////////////////////////////
  //                    Querying Value Information                    //
  //////////////////////////////////////////////////////////////////////

  DWORD value_type = 0;
  DWORD size_of_buffer = 0;

  LSTATUS query_status =
    RegQueryValueExA(hKey,           // [in]                HKEY    hKey,
                     "Path",         // [in, optional]      LPCSTR  lpValueName,
                     0,              // LPDWORD lpReserved,
                     &value_type,    // [out, optional]     LPDWORD lpType,
                     0,              // [out, optional]     LPBYTE  lpData,
                     &size_of_buffer // [in, out, optional] LPDWORD lpcbData
    );

  if (ERROR_FILE_NOT_FOUND == query_status) {
    return RegistryQuery{ ERROR_FILE_NOT_FOUND,
                          "Value not found for key.",
                          "" };
  }

  // Catch all other errors of RegQueryValueExA
  if (ERROR_SUCCESS != query_status) {
    return RegistryQuery{ query_status, "RegQueryValueExA failed.", "" };
  }

  if (REG_SZ != value_type) {
    return RegistryQuery{ 1, "Registry value not a string type.", "" };
  }

  //////////////////////////////////////////////////////////////////////
  //                      Getting the hKey Value                       //
  //////////////////////////////////////////////////////////////////////

  // Registry key may or may not be stored with a null terminator
  // add one just in case
  char* szPath = static_cast<char*>(calloc(1, size_of_buffer + 1));

  if (NULL == szPath) {
    return RegistryQuery{ 1, "Failed to allocate memory.", "" };
  }

  LSTATUS status_get_value =
    RegGetValueA(hKey,           // [in]                HKEY    hkey,
                 0,              // [in, optional]      LPCSTR  lpSubKey,
                 sub_value,      // [in, optional]      LPCSTR  lpValue,
                 RRF_RT_REG_SZ,  // [in, optional]      DWORD   dwFlags,
                 &value_type,    // [out, optional]     LPDWORD pdwType,
                 (void*)szPath,  // [out, optional]     PVOID   pvData,
                 &size_of_buffer // [in, out, optional] LPDWORD pcbData
    );

  if (ERROR_SUCCESS == status_get_value) {
    return RegistryQuery{ 0, "", std::string(szPath) };
  } else {
    return RegistryQuery{ status_get_value, "Could not get registry key.", "" };
  }
}

std::string
GetTrackIRDllFolderFromRegistry()
{
  std::string dll_path;

  RegistryQuery path = GetStringFromRegistry(
    HKEY_CURRENT_USER,
    "Software\\NaturalPoint\\NATURALPOINT\\NPClient Location",
    "Path");

  if (0 == path.result) {
    dll_path = path.value;
    spdlog::info("Acquired DLL location from registry.");
  } else {
    spdlog::error(
      "Could not find registry path. NP TrackIR may not be installed. To "
      "fix error, try specifying the folder of the \"NPClient64.dll\" in "
      "edit->settings.");
    spdlog::info("path.result: {}\n"
                 "path.resultString: {}",
                 path.result,
                 path.result_string);
  }

  return dll_path;
}

#endif /* TRACKIRMOUSE_REGISTRY_HPP */
