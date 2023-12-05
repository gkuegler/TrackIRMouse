#ifndef TRACKIRMOUSE_WATCHDOG_H
#define TRACKIRMOUSE_WATCHDOG_H

#include "windows-wrapper.hpp"

#include <atomic>
#include <exception>
#include <format>

#include "Log.hpp"

class Handle
{
public:
  HANDLE handle = INVALID_HANDLE_VALUE;
  Handle(HANDLE h) { handle = h; };

  Handle() = delete;                        // default constructor
  Handle(const Handle&) = delete;           // copy constructor
  Handle& operator=(Handle other) = delete; // copy assignment constructor

  ~Handle() { CloseHandle(handle); };
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
  void HandleConnection(Handle hPipe);
  std::string HandleMsg(std::string);
};

#endif /* TRACKIRMOUSE_WATCHDOG_H */
