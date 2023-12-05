#ifndef TRACKIRMOUSE_REGISTRY_HPP
#define TRACKIRMOUSE_REGISTRY_HPP

#include "windows-wrapper.hpp"

#include <string>

#include "log.hpp"

// typedef struct RegistryQuery_
// {
//   int result;
//   std::string result_string;
//   std::string value;
// } RegistryQuery;

/**
 * [GetStringFromRegistry description]
 * @param  parent_key [description]
 * @param  sub_key     [description]
 * @param  sub_value   [description]
 * @return            [description]
 */
std::string
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
    std::runtime_error("Registry key not found.");
  }
  // Catch all other errors
  else if (ERROR_SUCCESS != status_key_open) {
    std::runtime_error("Could not open registry key.");
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
    std::runtime_error("Value not found for key.");
  }

  // Catch all other errors of RegQueryValueExA
  if (ERROR_SUCCESS != query_status) {
    std::runtime_error(std::format(
      "RegQueryValueExA failed with error code: {}.", query_status));
  }

  if (REG_SZ != value_type) {
    std::runtime_error("Registry value not a string type.");
  }

  //////////////////////////////////////////////////////////////////////
  //                      Getting the hKey Value                       //
  //////////////////////////////////////////////////////////////////////

  // Registry key may or may not be stored with a null terminator
  // add one just in case
  char* szPath = static_cast<char*>(calloc(1, size_of_buffer + 1));

  if (NULL == szPath) {
    std::runtime_error("Failed to allocate memory for result buffer.");
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
    return std::string(szPath);
  } else {
    std::runtime_error(
      std::format("Could not retrieve registry key value with error code: {}.",
                  status_get_value));
  }
}

#endif /* TRACKIRMOUSE_REGISTRY_HPP */
