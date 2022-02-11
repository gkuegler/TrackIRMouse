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

#include "Watchdog.hpp"

#include <windows.h>

#include "Constants.h"
#include "Log.hpp"

#define BUFSIZE 512

namespace WatchDog {
bool g_bPauseTracking = false;

HANDLE StartWatchdog() {
  HANDLE hThread = INVALID_HANDLE_VALUE;
  DWORD dwThreadId = 0;
  LPCSTR eventName = "WatchdogInitThread";

  // Create an event for the thread to signal on
  // to ensure pipe initialization occurs before continuing.
  // This mostly matters so that the print statements
  // of the pipe initialization are not mixed in with the rest of the program
  HANDLE hEvent = CreateEventA(NULL, TRUE, 0, eventName);

  hThread = CreateThread(NULL,           // no security attribute
                         0,              // default stack size
                         InstanceThread, // thread proc
                         &hEvent,        // thread parameter
                         0,              // not suspended
                         &dwThreadId     // returns thread ID
  );

  if (hThread == NULL) {
    spdlog::error("CreateThread failed, GLE={}.\n", GetLastError());
    return NULL;
  }

  // Wait for the thread to signal when
  // it's completed initialization
  if (hEvent) {
    DWORD result = WaitForSingleObject(hEvent,
                                       3000 // timeout in milliseconds
    );

    if (WAIT_FAILED == result)
      spdlog::error("{} Function failed with error code: {}",
                    "WaitForSingleObject", GetLastError());

    if (WAIT_TIMEOUT == result)
      spdlog::info(
          "{} Timed out on watchdog thread. Objects state is non signaled.",
          "WaitForSingleObject");
  }

  return hThread;
}

DWORD WINAPI InstanceThread(LPVOID param) {
  // Signal event when set up of the pipe completes,
  // allowing main program flow to continue
  HANDLE *phEvent = reinterpret_cast<HANDLE *>(param);

  HANDLE hPipe = INVALID_HANDLE_VALUE;

  LPCSTR lpszPipename = "\\\\.\\pipe\\watchdog";

  // The main loop creates an instance of the named pipe and
  // then waits for a client to connect to it. When the client
  // connects, a thread is created to handle communications
  // with that client, and this loop is free to wait for the
  // next client connect request. It is an infinite loop.

  // May throw std::bad_alloc
  std::unique_ptr<SECURITY_DESCRIPTOR> pSD(new SECURITY_DESCRIPTOR);
  SECURITY_ATTRIBUTES sa;

  // The InitializeSecurityDescriptor function initializes a security descriptor
  // to have no system access control list(SACL), no discretionary access
  // control list(DACL), no owner, no primary group, and all control flags set
  // to FALSE(NULL). Thus, except for its revision level, it is empty
  if (NULL ==
      InitializeSecurityDescriptor(pSD.get(), SECURITY_DESCRIPTOR_REVISION)) {
    spdlog::error("InitializeSecurityDescriptor Error. GLE=%u\n",
                  GetLastError());
    return -1;
  }

  // Add the Access Control List (ACL) to the security descriptor
  // TODO: make this more robust, maybe limit to just the user process?
#pragma warning(disable : 6248) // Allow all unrestricted access to the pipe
  if (!SetSecurityDescriptorDacl(
          pSD.get(),
          TRUE,       // bDaclPresent flag
          (PACL)NULL, // if a NULL DACL is assigned to the security descriptor,
                      // all access is allowed
          FALSE))     // not a default DACL
  {
    spdlog::error("SetSecurityDescriptorDacl Error %u\n", GetLastError());
    // goto Cleanup;
  }
#pragma warning(default : 4700)

  // Initialize a security attributes structure.
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = pSD.get();
  sa.bInheritHandle = FALSE;

  // spdlog::info("\nPipe Server: Main thread awaiting client connection
  // on {}\n", lpszPipename);

  hPipe = CreateNamedPipeA(lpszPipename,               // pipe name
                           PIPE_ACCESS_DUPLEX,         // read/write access
                           PIPE_TYPE_MESSAGE |         // message type pipe
                               PIPE_READMODE_MESSAGE | // message-read mode
                               PIPE_WAIT,              // blocking mode
                           PIPE_UNLIMITED_INSTANCES,   // max. instances
                           BUFSIZE,                    // output buffer size
                           BUFSIZE,                    // input buffer size
                           0,                          // client time-out
                           &sa); // default security attribute

  if (hPipe == INVALID_HANDLE_VALUE) {
    spdlog::error("CreateNamedPipe failed, GLE={}.\n", GetLastError());
    return -1;
  }

  if (SetEvent(*phEvent) == 0) {
    spdlog::error("Could not set Watchdog Event with error code: {}\n",
                  GetLastError());
  }

  // Serve errors handled in implementation
  Serve(hPipe);

  return 0;
}

void Serve(HANDLE hPipe) {
  // This is a lot of Windows boilerplate code
  HANDLE hHeap = GetProcessHeap();
  char *pchRequest =
      (char *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, BUFSIZE * sizeof(char));
  char *pchReply =
      (char *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, BUFSIZE * sizeof(char));

  DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
  BOOL bSuccess = FALSE;

  // Do some extra error checking since the app will keep running even if this
  // thread fails.

  if (pchRequest == NULL) {
    spdlog::info("\nERROR - Pipe Server Failure:\n");
    spdlog::info("   Serve got an unexpected NULL heap allocation.\n");
    spdlog::info("   Serve exitting.\n");
    if (pchReply != NULL)
      HeapFree(hHeap, 0, pchReply);
    return;
  }

  if (pchReply == NULL) {
    spdlog::info("\nERROR - Pipe Server Failure:\n");
    spdlog::info("   Serve got an unexpected NULL heap allocation.\n");
    spdlog::info("   Serve exitting.\n");
    if (pchRequest != NULL)
      HeapFree(hHeap, 0, pchRequest);
    return;
  }

  while (1) {
    spdlog::info("Waiting on client connection...\n");

    if (ConnectNamedPipe(hPipe, NULL) == 0) {
      spdlog::error("ConnectNamedPipe failed, GLE={}.\n", GetLastError());
      break;
    }
    spdlog::info("Client Connected!\n");

    while (1) {
      // Read client requests from the pipe. This simplistic code only allows
      // messages up to BUFSIZE characters in length.
      bSuccess = ReadFile(hPipe,      // handle to pipe
                          pchRequest, // buffer to receive data
                          BUFSIZE * sizeof(unsigned char), // size of buffer
                          &cbBytesRead, // number of bytes read
                          NULL);        // not overlapped I/O

      if (!bSuccess || cbBytesRead == 0) {
        if (GetLastError() == ERROR_BROKEN_PIPE) {
          spdlog::info("Serve: client disconnected.\n");
          break;
        } else {
          spdlog::error("InstanceThread ReadFile failed, GLE={}.\n",
                        GetLastError());
          break;
        }
      }

      // spdlog::info("CLIENT MSG: {}\n", pchRequest);

      // Process the incoming message.
      HandleMsg(pchRequest, pchReply, &cbReplyBytes);

      Sleep(100);

      // Write the reply to the pipe.
      bSuccess = WriteFile(hPipe,        // handle to pipe
                           pchReply,     // buffer to write from
                           cbReplyBytes, // number of bytes to write
                           &cbWritten,   // number of bytes written
                           NULL          // not overlapped I/O
      );

      if (!bSuccess || cbReplyBytes != cbWritten) {
        spdlog::error("InstanceThread WriteFile failed, GLE={}.\n",
                      GetLastError());
      } else {
        spdlog::info("Number of Bytes Written: {}\n", cbReplyBytes);
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

  spdlog::info("InstanceThread exiting.\n");
  return;
}

VOID HandleMsg(const char *pchRequest, char *pchReply, LPDWORD pchBytes)
// Reads message and performs actions based on the content
// Writes a reply into the reply char*
{
  errno_t rslt = 1;

  spdlog::info("Client Request:\n{}\n", pchRequest);

  if (strcmp(pchRequest, "KILL") == 0) {
    rslt = strcpy_s(pchReply, BUFSIZE, "KL");
    if (system("taskkill /T /IM TrackIR5.exe") == -1)
      spdlog::info("Failed to kill TrackIR5 program with error code");
  } else if (strcmp(pchRequest, "HEARTBEAT") == 0) {
    rslt = strcpy_s(pchReply, BUFSIZE, "HB");
  } else if (strcmp(pchRequest, "PAUSE") == 0) {
    rslt = strcpy_s(pchReply, BUFSIZE, "PAUSE");
    g_bPauseTracking = !g_bPauseTracking;
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
    spdlog::info("strcpy_s failed, no outgoing message.\n");
    return;
  }
}

} // namespace WatchDog
#undef BUFSIZE
