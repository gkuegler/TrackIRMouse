#ifndef TRACKIRMOUSE_WATCHDOG_H
#define TRACKIRMOUSE_WATCHDOG_H

#include <Windows.h>

#include <atomic>
#include <exception>
#include <format>

#include "Log.hpp"

class Handle {
 public:
  HANDLE handle = INVALID_HANDLE_VALUE;
  Handle(HANDLE h) { handle = h; };

  Handle() = delete;                         // default constructor
  Handle(const Handle&) = delete;            // copy constructor
  Handle& operator=(Handle other) = delete;  // copy assignment constructor

  ~Handle() { CloseHandle(handle); };
};

// Single threaded pipe server
class PipeServer {
 public:
  PipeServer();
  //~PipeServer();
  void Serve(std::string);

 private:
  std::shared_ptr<spdlog::logger> logger;

  void HandleConnection(Handle hPipe);
  std::string HandleMsg(std::string);
};

#endif /* TRACKIRMOUSE_WATCHDOG_H */
