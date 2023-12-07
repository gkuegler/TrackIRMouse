#ifndef TRACKIRMOUSE_WATCHDOG_H
#define TRACKIRMOUSE_WATCHDOG_H

#include "windows-wrapper.hpp"

#include <atomic>
#include <exception>
#include <format>

#include "Log.hpp"

class WindowsHandle
{
public:
  HANDLE handle = INVALID_HANDLE_VALUE;
  WindowsHandle(HANDLE h) { handle = h; };

  WindowsHandle() = delete;                     // default constructor
  WindowsHandle(const WindowsHandle&) = delete; // copy constructor
  WindowsHandle& operator=(WindowsHandle other) =
    delete; // copy assignment constructor

  ~WindowsHandle() { CloseHandle(handle); };
};

// Single threaded pipe server
class PipeServer
{
private:
  std::shared_ptr<spdlog::logger> logger_;
  std::string full_path_;
  SECURITY_DESCRIPTOR pSD_;
  SECURITY_ATTRIBUTES sa_;

public:
  PipeServer(std::string name);

  void ServeOneClient();

private:
  void HandleConnection(WindowsHandle hPipe);
  std::string HandleMsg(std::string);
};

#endif /* TRACKIRMOUSE_WATCHDOG_H */
