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

#include "watchdog.hpp"

#include "log.hpp"
#include "track.hpp"

constexpr auto BUFSIZE = 512;

namespace WatchDog {

HANDLE InitializeWatchdog() {
  spdlog::trace("StartWatchdog");
  HANDLE hPipe = INVALID_HANDLE_VALUE;
  LPCSTR lpszPipename = "\\\\.\\pipe\\watchdog";

  // The main loop creates an instance of the named pipe and
  // then waits for a client to connect to it. When the client
  // connects, a thread is created to handle communications
  // with that client, and this loop is free to wait for the
  // next client connect request. It is an infinite loop.

  // Initialize Security Descriptor For Named Pipe
  // May throw std::bad_alloc
  std::unique_ptr<SECURITY_DESCRIPTOR> pSD(new SECURITY_DESCRIPTOR);
  SECURITY_ATTRIBUTES sa;

  // The InitializeSecurityDescriptor function initializes a security descriptor
  // to have no system access control list(SACL), no discretionary access
  // control list(DACL), no owner, no primary group, and all control flags set
  // to FALSE(NULL). Thus, except for its revision level, it is empty
  if (NULL ==
      InitializeSecurityDescriptor(pSD.get(), SECURITY_DESCRIPTOR_REVISION)) {
    spdlog::error(
        "Failed to initialize watchdog security descriptor with error code: {}",
        GetLastError());
    return NULL;
  }

  // Add the Access Control List (ACL) to the security descriptor
  // TODO: make this more robust, maybe limit to just the user process?
#pragma warning(disable : 6248)  // Allow all unrestricted access to the pipe
  if (!SetSecurityDescriptorDacl(
          pSD.get(),
          TRUE,        // bDaclPresent flag
          (PACL)NULL,  // if a NULL DACL is assigned to the security descriptor,
                       // all access is allowed
          FALSE))      // not a default DACL
  {
    spdlog::warn("SetSecurityDescriptorDacl Error {}", GetLastError());
    return NULL;
  }
#pragma warning(default : 4700)

  // Initialize a security attributes structure.
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = pSD.get();
  sa.bInheritHandle = FALSE;

  hPipe = CreateNamedPipeA(lpszPipename,                // pipe name
                           PIPE_ACCESS_DUPLEX,          // read/write access
                           PIPE_TYPE_MESSAGE |          // message type pipe
                               PIPE_READMODE_MESSAGE |  // message-read mode
                               PIPE_WAIT,               // blocking mode
                           // PIPE_UNLIMITED_INSTANCES,    // max. instances
                           0,        // max. instances
                           BUFSIZE,  // output buffer size
                           BUFSIZE,  // input buffer size
                           0,        // client time-out
                           &sa);     // default security attribute

  if (hPipe == INVALID_HANDLE_VALUE) {
    spdlog::warn("CreateNamedPipe failed, GLE={}.", GetLastError());
    return NULL;
  }

  return hPipe;
}

void Serve(HANDLE hPipe) {
  spdlog::trace("starting watchdog server");

  // Initialize send and returN buffers
  HANDLE hHeap = GetProcessHeap();
  char *pchRequest =
      (char *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, BUFSIZE * sizeof(char));
  char *pchReply =
      (char *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, BUFSIZE * sizeof(char));

  if (pchRequest == NULL) {
    spdlog::warn(
        "Pipe Server Failure. Failed to allocate request buffer"
        "Server exitting.");
    if (pchReply != NULL) {
      HeapFree(hHeap, 0, pchReply);
    }
    return;
  }

  if (pchReply == NULL) {
    spdlog::warn(
        "Pipe Server Failure. Failed to allocate reply buffer"
        "Server exitting.");
    if (pchRequest != NULL) {
      HeapFree(hHeap, 0, pchRequest);
    }
    return;
  }

  // Main serve loop
  while (1) {
    spdlog::info("waiting on client connection...");

    if (ConnectNamedPipe(hPipe, NULL) == 0) {
      spdlog::warn("ConnectNamedPipe failed, GLE={}.", GetLastError());
      break;
    }
    spdlog::info("client connected");

    while (1) {
      DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;

      // Read client requests from the pipe. This simplistic code only allows
      // messages up to BUFSIZE characters in length.
      BOOL bSuccess =
          ReadFile(hPipe,                            // handle to pipe
                   pchRequest,                       // buffer to receive data
                   BUFSIZE * sizeof(unsigned char),  // size of buffer
                   &cbBytesRead,                     // number of bytes read
                   NULL);                            // not overlapped I/O

      if (!bSuccess || cbBytesRead == 0) {
        if (GetLastError() == ERROR_BROKEN_PIPE) {
          spdlog::info("Server: client disconnected.");
          break;
        } else {
          spdlog::error("InstanceThread ReadFile failed, GLE={}.",
                        GetLastError());
          break;
        }
      }

      spdlog::debug("client message: {}", pchRequest);

      // Process the incoming message.
      HandleMsg(pchRequest, pchReply, &cbReplyBytes);

      Sleep(100);

      // Write the reply to the pipe.
      bSuccess = WriteFile(hPipe,         // handle to pipe
                           pchReply,      // buffer to write from
                           cbReplyBytes,  // number of bytes to write
                           &cbWritten,    // number of bytes written
                           NULL           // not overlapped I/O
      );

      if (!bSuccess || cbReplyBytes != cbWritten) {
        spdlog::error("InstanceThread WriteFile failed, GLE={}.",
                      GetLastError());
      } else {
        spdlog::info("Number of Bytes Written: {}", cbReplyBytes);
      }

      // Flush the pipe to allow the client to read the pipe's contents
      // before disconnecting. Then disconnect the pipe, and close the
      // handle to this pipe instance.

      FlushFileBuffers(hPipe);

      // zero out the message receive and reply character arrays
      memset(pchRequest, 0, BUFSIZE * sizeof(char));
      memset(pchReply, 0, BUFSIZE * sizeof(char));
    }

    DisconnectNamedPipe(hPipe);
    Sleep(500);
  }

  HeapFree(hHeap, 0, pchRequest);
  HeapFree(hHeap, 0, pchReply);

  return;
}

VOID HandleMsg(const char *pchRequest, char *pchReply, LPDWORD pchBytes)
// Reads message and performs actions based on the content.
// Writes a reply into the reply char* buffer.
{
  errno_t rslt = 1;

  if (strcmp(pchRequest, "KILL") == 0) {
    rslt = strcpy_s(pchReply, BUFSIZE, "KL");
    if (system("taskkill /T /IM TrackIR5.exe") == -1)
      spdlog::error("Failed to kill TrackIR5 program with error code");
  } else if (strcmp(pchRequest, "HEARTBEAT") == 0) {
    rslt = strcpy_s(pchReply, BUFSIZE, "HEARTBEAT\0");
  } else if (strcmp(pchRequest, "PAUSE") == 0) {
    rslt = strcpy_s(pchReply, BUFSIZE, "PAUSE\0");
    track::TrackToggle();
  } else {
    rslt = strcpy_s(pchReply, BUFSIZE, "NONE");
  }

  // Check the outgoing message to make sure it's not too long for the buffer.
  if (rslt == 0) {
    *pchBytes = strlen(pchReply) + 1;
    return;
  }

  else {
    *pchBytes = 0;
    pchReply[0] = 0;
    spdlog::warn("strcpy_s failed, no outgoing message.");
    return;
  }
}

}  // namespace WatchDog
