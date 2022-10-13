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
 *          no signs of burnout.
 *
 * --License Boilerplate Placeholder--
 *
 * TODO section:
 * currently no protection against two messages that are sent so fast that the
 * server reads two messages in one go therefore dropping a message or erroring
 * out
 *
 */

#include "pipeserver.hpp"

#include "track.hpp"

constexpr size_t BUFSIZE = 512 * sizeof(unsigned char);

PipeServer::PipeServer() { logger = mylogging::MakeLoggerFromStd("watchdog"); }

// Watchdog::~Watchdog() {
//  nothing to clean up explicitly
//}

void PipeServer::Serve(std::string name) {
  HANDLE hPipe = INVALID_HANDLE_VALUE;
  std::string full_path = "\\\\.\\pipe\\" + name;
  // LPCSTR lpszPipename = "\\\\.\\pipe\\watchdog";

  // The main loop creates an instance of the named pipe and
  // then waits for a client to connect to it. When the client
  // connects, a thread is created to handle communications
  // with that client, and this loop is free to wait for the
  // next client connect request. It is an infinite loop.

  // Initialize Security Descriptor For Named Pipe
  // May throw std::bad_alloc
  std::unique_ptr<SECURITY_DESCRIPTOR> pSD(new SECURITY_DESCRIPTOR);

  // The InitializeSecurityDescriptor function initializes a security descriptor
  // to have no system access control list(SACL), no discretionary access
  // control list(DACL), no owner, no primary group, and all control flags set
  // to FALSE(NULL). Thus, except for its revision level, it is empty
  if (NULL ==
      InitializeSecurityDescriptor(pSD.get(), SECURITY_DESCRIPTOR_REVISION)) {
    throw std::runtime_error(std::format(
        "Failed to initialize watchdog security descriptor with error code: {}",
        GetLastError()));
  }

  // Add the Access Control List (ACL) to the security descriptor
  // TODO: make this more robust, maybe limit to just user processes?
#pragma warning(disable : 6248)  // Allow all unrestricted access to the pipe
  if (!SetSecurityDescriptorDacl(
          pSD.get(),
          TRUE,        // bDaclPresent flag
          (PACL)NULL,  // if a NULL DACL is assigned to the security descriptor,
                       // all access is allowed
          FALSE))      // not a default DACL
  {
    throw std::runtime_error(
        std::format("SetSecurityDescriptorDacl Error {}", GetLastError()));
  }
#pragma warning(default : 4700)

  // Initialize a security attributes structure.
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = pSD.get();
  sa.bInheritHandle = FALSE;

  while (1) {
    // Create a new named pipe instance for a new client to handle.
    // Setting the max number of instances to a reasonable level
    // will prevent rogue clients from freezing my computer.
    // If I was feeling brave I could use PIPE_UNLIMITED_INSTANCES.
    HANDLE hPipe =
        CreateNamedPipeA(full_path.c_str(),           // pipe name
                         PIPE_ACCESS_DUPLEX,          // read/write access
                         PIPE_TYPE_MESSAGE |          // message type pipe
                             PIPE_READMODE_MESSAGE |  // message-read mode
                             PIPE_WAIT,               // blocking mode
                         10,       // max. instances, i.e max number of clients
                         BUFSIZE,  // output buffer size
                         BUFSIZE,  // input buffer size
                         0,        // client time-out
                         &sa);     // default security attribute

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
    }

    // Wait for the client to connect; if it succeeds,
    // the function returns a nonzero value. If the function
    // returns zero, GetLastError returns ERROR_PIPE_CONNECTED.
    bool connected = ConnectNamedPipe(hPipe, NULL)
                         ? TRUE
                         : (GetLastError() == ERROR_PIPE_CONNECTED);

    if (connected) {
      // printf("client connected, creating a processing thread.\n");
      logger->debug("client connected");
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
void PipeServer::HandleConnection(Handle pipe) {
  logger->trace("starting watchdog server");

  // Initialize send and return buffers
  std::vector<char> request_buffer(BUFSIZE, '\0');
  std::vector<char> reply_buffer(BUFSIZE, '\0');

  DWORD bytes_read_count = 0;

  // Read client requests from the pipe. This simplistic code only allows
  // messages up to BUFSIZE characters in length.
  BOOL read_result = ReadFile(pipe.handle,            // handle to pipe
                              request_buffer.data(),  // buffer to receive data
                              BUFSIZE,                // size of buffer
                              &bytes_read_count,      // number of bytes read
                              NULL);                  // not overlapped I/O

  if (!read_result || bytes_read_count == 0) {
    if (GetLastError() == ERROR_BROKEN_PIPE) {
      logger->debug("client disconnected");
      return;
    } else {
      logger->error("ReadFile failed, GLE={}.", GetLastError());
      return;
    }
  }

  std::string request(request_buffer.data());
  logger->debug("client message received: {}", request);

  // Process the incoming message.
  const auto reply = HandleMsg(request);
  DWORD reply_byte_count = reply.length();
  DWORD bytes_written_count = 0;

  logger->debug("reply message formulated: {}", reply);

  // Write the reply to the pipe.
  BOOL write_result =
      WriteFile(pipe.handle,           // handle to pipe
                reply.c_str(),         // buffer to write from
                reply_byte_count,      // number of bytes to write
                &bytes_written_count,  // number of bytes written
                NULL                   // not overlapped I/O
      );

  if (!write_result || reply_byte_count != bytes_written_count) {
    logger->error("WriteFile failed, GLE={}.", GetLastError());
  }

  // Flush the pipe to allow the client to read the pipe's contents
  // before disconnecting. Then disconnect the pipe, and close the
  // handle to this pipe instance.
  // Client should poll until pipe becomes available.
  FlushFileBuffers(pipe.handle);
  DisconnectNamedPipe(pipe.handle);
  // Handle closed on destruction.

  return;
}

// Reads message and performs actions based on the content.
// Writes a reply into the reply char* buffer.
std::string PipeServer::HandleMsg(std::string request) {
  if (request == "KILL") {
    if (system("taskkill /T /IM TrackIR5.exe") == -1) {
      logger->error("Failed to kill TrackIR5 program");
    }
    return "KL";
  } else if (request == "HEARTBEAT") {
    return "HEARTBEAT";
  } else if (request == "PAUSE") {
    track::Toggle();
    return "PAUSE";
  } else {
    return "NONE";
  }
}
