/**
 * Thread to control head tracker from a namedpipe.
 *
 * Send explicit commands via a named pipe to internally pause or kill track our
 * program.
 *  - PAUSE: suspends fetching of track data and mouse send input commands
 *  - KILL: Shuts down the NPTrackIR program to turn of the TrackIR5.
 *          I had a problem with a then deffective model where the internal
 *          LED's burnt out and thought I need a way to shut down the device
 *          with voice commands. I never really use this command and since then
 *          I have ran my TrackI5 60+ hours per week for over a year with still
 *          no signs of led burnout.
 *
 * --License Boilerplate Placeholder--
 *
 */

#include "pipeserver.hpp"

#include "messages.hpp"
#include "mouse-modes.hpp"
#include "trackers.hpp"

#include <format>

constexpr size_t BUFSIZE = 512 * sizeof(unsigned char);

PipeServer::PipeServer(std::string name)
{
  logger_ = mylogging::GetClonedLogger("pipeserver");
  std::string full_path = "\\\\.\\pipe\\" + name;

  // The InitializeSecurityDescriptor function initializes a security descriptor
  // to have no system access control list(SACL), no discretionary access
  // control list(DACL), no owner, no primary group, and all control flags set
  // to FALSE(NULL). Thus, except for its revision level, it is empty
  if (NULL ==
      InitializeSecurityDescriptor(&pSD_, SECURITY_DESCRIPTOR_REVISION)) {
    throw std::runtime_error(std::format(
      "Failed to initialize watchdog security descriptor with error code: {}",
      GetLastError()));
  }

  // Add the Access Control List (ACL) to the security descriptor
  // TODO: make this safer, maybe limit to just user processes?
#pragma warning(disable : 6248) // Allow all unrestricted access to the pipe
  if (!SetSecurityDescriptorDacl(
        &pSD_,
        TRUE,       // bDaclPresent flag
        (PACL)NULL, // if a NULL DACL is assigned to the security descriptor,
                    // all access is allowed
        FALSE))     // not a default DACL
  {
    throw std::runtime_error(std::format(
      "SetSecurityDescriptorDacl Initialization Error {}", GetLastError()));
  }

  // Initialize a security attributes structure.
  sa_.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa_.lpSecurityDescriptor = &pSD_;
  sa_.bInheritHandle = FALSE;
}

void
PipeServer::ServeOneClient()
{
  // Create a new named pipe instance for a new client to handle_.
  // Setting the max number of instances to a reasonable level
  // will prevent rogue clients from freezing my computer.
  // If I was feeling brave I could use PIPE_UNLIMITED_INSTANCES.
  HANDLE hPipe =
    CreateNamedPipeA(full_path_.c_str(),       // pipe name
                     PIPE_ACCESS_DUPLEX,       // read/write access
                     PIPE_TYPE_MESSAGE |       // message type pipe
                       PIPE_READMODE_MESSAGE | // message-read mode
                       PIPE_WAIT,              // blocking mode
                     10,      // max. instances, i.e max number of clients
                     BUFSIZE, // output buffer size
                     BUFSIZE, // input buffer size
                     0,       // client time-out
                     &sa_);   // default security attribute

  if (hPipe == INVALID_HANDLE_VALUE) {
    DWORD gle = GetLastError();
    if (gle == ERROR_PIPE_BUSY) {
      throw std::runtime_error(
        "CreateNamedPipe failed, all instances are busy.");
    } else if (gle == ERROR_INVALID_PARAMETER) {
      throw std::runtime_error(
        "CreateNamedPipe failed, function called with incorrect "
        "parameters.");

    } else {
      throw std::runtime_error(
        std::format("CreateNamedPipe failed, GLE={}.", gle));
    }

    // Wait for the client to connect; if it succeeds,
    // the function returns a nonzero value. If the function
    // returns zero, GetLastError returns ERROR_PIPE_CONNECTED.
    bool connected = ConnectNamedPipe(hPipe, NULL)
                       ? TRUE
                       : (GetLastError() == ERROR_PIPE_CONNECTED);

    if (connected) {
      logger_->debug("client connected");
      HandleConnection(hPipe);

      // https://learn.microsoft.com/en-us/windows/win32/ipc/multithreaded-pipe-server
      // Create a thread for this client.
      // hThread = CreateThread(NULL,            // no security attribute
      //                        0,               // default stack size
      //                        InstanceThread,  // thread proc
      //                        (LPVOID)hPipe,   // thread parameter
      //                        0,               // not suspended
      //                        &dwThreadId);    // returns thread ID

      // if (hThread == NULL) {
      //_tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError());
      // return -1;
      //} else
      // CloseHandle(hThread);
      //} else
      // The client could not connect, so close the pipe.
      // CloseHandle(hPipe);
    }
  }
}

// Handle a client on an instance of a main pipe.
// Handle closed on destruction.
void
PipeServer::HandleConnection(Handle pipe)
{
  logger_->trace("starting pipeserver");

  // Initialize send and return buffers
  std::vector<char> request_buffer(BUFSIZE, '\0');
  std::vector<char> reply_buffer(BUFSIZE, '\0');

  DWORD bytes_read_count = 0;

  // Read client requests from the pipe. This simplistic code only allows
  // messages up to BUFSIZE characters in length.
  BOOL read_result = ReadFile(pipe.handle,           // handle_ to pipe
                              request_buffer.data(), // buffer to receive data
                              BUFSIZE,               // size of buffer
                              &bytes_read_count,     // number of bytes read
                              NULL);                 // not overlapped I/O

  if (!read_result || bytes_read_count == 0) {
    if (GetLastError() == ERROR_BROKEN_PIPE) {
      logger_->debug("client disconnected");
      return;
    } else {
      logger_->error("ReadFile failed, GLE={}.", GetLastError());
      return;
    }
  }

  std::string request(request_buffer.data());
  logger_->debug("client message received: {}", request);

  // Process the incoming message.
  const auto reply = HandleMsg(request);
  DWORD reply_byte_count = reply.length();
  DWORD bytes_written_count = 0;

  logger_->debug("reply message formulated: {}", reply);

  // Write the reply to the pipe.
  BOOL write_result = WriteFile(pipe.handle,      // handle_ to pipe
                                reply.c_str(),    // buffer to write from
                                reply_byte_count, // number of bytes to write
                                &bytes_written_count, // number of bytes written
                                NULL                  // not overlapped I/O
  );

  if (!write_result || reply_byte_count != bytes_written_count) {
    logger_->error("WriteFile failed, GLE={}.", GetLastError());
  }

  // Flush the pipe to allowe the client to read the pipe's contents
  // before disconnecting. Then disconnect the pipe, and close the
  // handle_ to this pipe instance.
  // Client should poll until pipe becomes available.
  FlushFileBuffers(pipe.handle);
  DisconnectNamedPipe(pipe.handle);
  // Handle closed on destruction.

  return;
}
// clang-format off
// Reads message and performs actions based on the content.
// Writes a reply into the reply char* buffer.
std::string PipeServer::HandleMsg(std::string request) {
  if (request == "KILL") {
    if (system("taskkill /T /IM TrackIR5.exe") == -1) {
      logger_->error("Failed to kill TrackIR5 program");
    }
    return request;
  } else if (request == "HEARTBEAT") {
    return request;
  } else if (request == "PAUSE") {
    SendThreadMessage(msgcode::toggle_tracking);
    return request;
  } else if (request == "SCROLL_LEFT_SMALL") {
    SendThreadMessage(msgcode::set_mode, "", static_cast<long >(mouse_mode::scrollbar_left_small));
		spdlog::info("set alternate mouse mode: SCROLL_LEFT_SMALL");
    return request;
  } else if (request == "SCROLL_LEFT_MINI_MAP") {
    SendThreadMessage(msgcode::set_mode, "", static_cast<long >(mouse_mode::scrollbar_left_mini_map));
		spdlog::info("set alternate mouse mode: SCROLL_LEFT_MINI_MAP");
    return request;
  } else if (request == "SCROLL_RIGHT_SMALL") {
    SendThreadMessage(msgcode::set_mode, "", static_cast<long >(mouse_mode::scrollbar_right_small));
		spdlog::info("set alternate mouse mode: SCROLL_RIGHT_SMALL");
    return request;
  } else if (request == "SCROLL_RIGHT_MINI_MAP") {
    SendThreadMessage(msgcode::set_mode, "", static_cast<long >(mouse_mode::scrollbar_right_mini_map));
		spdlog::info("set alternate mouse mode: SCROLL_RIGHT_MINI_MAP");
    return request;
  } else if (request == "SCROLL_HOLD_X") {
    SendThreadMessage(msgcode::set_mode, "", static_cast<long >(mouse_mode::scrollbar_hold_x));
		spdlog::info("set alternate mouse mode: SCROLL_HOLD_X");
    return request;
  } else {
    return "NONE";
  }
}
