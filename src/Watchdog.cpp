#include <windows.h> 
#include <stdio.h> 
#include <tchar.h>
#include <strsafe.h>
#include <iostream>
#include <aclapi.h>

#define FMT_HEADER_ONLY
#include <fmt\format.h>
#include "Log.h"

#include "Constants.h"

#include "Watchdog.h"


#define BUFSIZE 512

namespace WatchDog
{

HANDLE WD_StartWatchdog()
{
        
    HANDLE hThread = INVALID_HANDLE_VALUE;
    DWORD  dwThreadId = 0;
    LPCSTR event_name = "WatchdogInitThread";

    // Create an event for the thread to signal on
    // to ensure pipe initialization occurs before continuing.
    // This mostly matters so that the print statements
    // of the pipe initialization are not mixed in with the rest of the program
    HANDLE hEvent = CreateEventA(
        NULL,
        TRUE,
        0,
        event_name
    );

    hThread = CreateThread(
        NULL,              // no security attribute 
        0,                 // default stack size 
        WD_InstanceThread,  // thread proc
        &hEvent,              // thread parameter
        0,                 // not suspended 
        &dwThreadId         // returns thread ID
    );      

    if (hThread == NULL)
    {
        logToWix(fmt::format("CreateThread failed, GLE={}.\n", GetLastError()));
        return NULL;
    }
        

    // Wait for the thread to signal when
    // it's completed initialization
    if (hEvent)
    {
        BOOL result = WaitForSingleObject(
            hEvent,
            3000         // timeout in milliseconds
        );

    }

    return hThread;
}

DWORD WINAPI WD_InstanceThread(LPVOID param)
{
    // Signal event when set up of the pipe completes,
    // allowing main program flow to continue
    HANDLE* phEvent = reinterpret_cast<HANDLE*>(param);

    HANDLE hPipe = INVALID_HANDLE_VALUE;
    HANDLE hHeap = GetProcessHeap();

    LPCSTR lpszPipename = "\\\\.\\pipe\\watchdog";
    DWORD dwServeRslt = -1;
    int lala = 0;
    BOOL result = 0;

    // The main loop creates an instance of the named pipe and 
    // then waits for a client to connect to it. When the client 
    // connects, a thread is created to handle communications 
    // with that client, and this loop is free to wait for the
    // next client connect request. It is an infinite loop.

    //Security descriptor needs to go on the Heap
    //PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, SECURITY_DESCRIPTOR_MIN_LENGTH);
    std::unique_ptr<SECURITY_DESCRIPTOR> pSD(new SECURITY_DESCRIPTOR);
    SECURITY_ATTRIBUTES sa;
    HKEY hkSub = NULL;

    if (NULL == pSD)
    {
        logToWix(fmt::format("Heap allocation for security descriptor failed. GLE=%u\n", GetLastError()));
        return 1;
    }

    // The InitializeSecurityDescriptor function initializes a security descriptor to have no system
    // access control list(SACL), no discretionary access control list(DACL), no owner, no primary group,
    // and all control flags set to FALSE(NULL). Thus, except for its revision level, it is empty
    if (NULL == InitializeSecurityDescriptor(pSD.get(), SECURITY_DESCRIPTOR_REVISION))
    {
        logToWix(fmt::format("InitializeSecurityDescriptor Error. GLE=%u\n", GetLastError()));
        //HeapFree(hHeap, 0, pSD);
        return 1;
    }


    // Add the Access Control List (ACL) to the security descriptor. 
    
//Allow all unrestricted access to the pipe
#pragma warning(disable:6248)
    if (!SetSecurityDescriptorDacl(
        pSD.get(),
        TRUE,        // bDaclPresent flag   
        (PACL)NULL,  // if a NULL DACL is assigned to the security descriptor, all access is allowed
        FALSE))      // not a default DACL 
    {
        logToWix(fmt::format("SetSecurityDescriptorDacl Error %u\n", GetLastError()));
        //goto Cleanup;
    }
#pragma warning(default:4700)

    // Initialize a security attributes structure.
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSD.get();
    sa.bInheritHandle = FALSE;

    logToWix(fmt::format("\nPipe Server: Main thread awaiting client connection on {}\n", lpszPipename));

    hPipe = CreateNamedPipeA(
        lpszPipename,             // pipe name 
        PIPE_ACCESS_DUPLEX,       // read/write access 
        PIPE_TYPE_MESSAGE |       // message type pipe 
        PIPE_READMODE_MESSAGE |   // message-read mode 
        PIPE_WAIT,                // blocking mode 
        PIPE_UNLIMITED_INSTANCES, // max. instances  
        BUFSIZE,                  // output buffer size 
        BUFSIZE,                  // input buffer size 
        0,                        // client time-out 
        &sa);                     // default security attribute 

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        logToWix(fmt::format("CreateNamedPipe failed, GLE={}.\n", GetLastError()));
        return -1;
    }

    result = SetEvent(*phEvent);
    lala = WD_Serve(hPipe);

    //HeapFree(hHeap, 0, pSD);
    return 0;
}

int WD_Serve(HANDLE hPipe)
{
    // This is a lot of Windows boilerplate code
    HANDLE hHeap = GetProcessHeap();
    char* pchRequest = (char*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, BUFSIZE * sizeof(char));
    char* pchReply = (char*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, BUFSIZE * sizeof(char));

    DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
    BOOL bSuccess = FALSE;

    // Do some extra error checking since the app will keep running even if this
    // thread fails.

    if (pchRequest == NULL)
    {
        logToWix(fmt::format("\nERROR - Pipe Server Failure:\n"));
        logToWix(fmt::format("   Serve got an unexpected NULL heap allocation.\n"));
        logToWix(fmt::format("   Serve exitting.\n"));
        if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
        return (DWORD)-1;
    }

    if (pchReply == NULL)
    {
        logToWix(fmt::format("\nERROR - Pipe Server Failure:\n"));
        logToWix(fmt::format("   Serve got an unexpected NULL heap allocation.\n"));
        logToWix(fmt::format("   Serve exitting.\n"));
        if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
        return (DWORD)-1;
    }

    while (1)
    {
        logToWix(fmt::format("Waiting on client connection...\n"));

        if (ConnectNamedPipe(hPipe, NULL) == 0)
        {
            logToWix(fmt::format("ConnectNamedPipe failed, GLE={}.\n", GetLastError()));
            break;
        }
        logToWix(fmt::format("Client Connected!\n"));

        while (1)
        {
            // Read client requests from the pipe. This simplistic code only allows messages
            // up to BUFSIZE characters in length.
            bSuccess = ReadFile(
                hPipe,        // handle to pipe 
                pchRequest,    // buffer to receive data 
                BUFSIZE * sizeof(unsigned char), // size of buffer 
                &cbBytesRead, // number of bytes read 
                NULL);        // not overlapped I/O 

            if (!bSuccess || cbBytesRead == 0)
            {
                if (GetLastError() == ERROR_BROKEN_PIPE)
                {
                    logToWix(fmt::format("Serve: client disconnected.\n"));
                    break;
                }
                else
                {
                    logToWix(fmt::format("WD_InstanceThread ReadFile failed, GLE={}.\n", GetLastError()));
                    break;
                }
            }

            //logToWix(fmt::format("CLIENT MSG: {}\n", pchRequest));

            // Process the incoming message.
            WD_HandleMsg(pchRequest, pchReply, &cbReplyBytes);

            //_tcscpy_s(pchReply, BUFSIZE, TEXT("ACK"));
            //cbReplyBytes = BUFSIZE * sizeof(char);
            Sleep(100);

            // Write the reply to the pipe. 

            bSuccess = WriteFile(
                hPipe,         // handle to pipe 
                pchReply,      // buffer to write from
                //pchRequest,
                cbReplyBytes,  // number of bytes to write 
                & cbWritten,   // number of bytes written 
                NULL           // not overlapped I/O 
            );        

            if (!bSuccess || cbReplyBytes != cbWritten)
            {
                logToWix(fmt::format("WD_InstanceThread WriteFile failed, GLE={}.\n", GetLastError()));
            }
            else
            {
                logToWix(fmt::format("Number of Bytes Written: {}\n", cbReplyBytes));
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

    logToWix(fmt::format("WD_InstanceThread exiting.\n"));
    return (DWORD)1;

}

VOID WD_HandleMsg(const char* pchRequest, char* pchReply, LPDWORD pchBytes)
    // Reads message and performs actions based on the content
    // Writes a reply into the reply char*
{
    const char* msg_ack = "ACK";
    errno_t rslt = 1;

    logToWix(fmt::format("Client Request:\n{}\n", pchRequest));

    if (strcmp(pchRequest, "KILL") == 0)
    {
        rslt = strcpy_s(pchReply, BUFSIZE, "KL");
        int retval = system("taskkill /T /IM TrackIR5.exe");
    }
    else if (strcmp(pchRequest, "HEARTBEAT") == 0)
    {
        rslt = strcpy_s(pchReply, BUFSIZE, "HB");
    }
    else if (strcmp(pchRequest, "PAUSE") == 0)
    {
        rslt = strcpy_s(pchReply, BUFSIZE, "PAUSE");
        g_pauseTracking = !g_pauseTracking;
    }
    else
    {
        rslt = strcpy_s(pchReply, BUFSIZE, "NONE");
    }

    // Check the outgoing message to make sure it's not too long for the buffer.
    if (rslt == 0)
    {
        *pchBytes = strlen(pchReply) + 1;
        return;
    }

    else
    {
        *pchBytes = 0;
        pchReply[0] = 0;
        logToWix(fmt::format("strcpy_s failed, no outgoing message.\n"));
        return;
    }
}

}
#undef BUFSIZE



// DESIGN DECISIONS
/*
should I implement tcp where my program can ask for a reply? YES & YES
    or just resend a message every so often? <<<< this option
*/

// POTENTIAL PROBLEMS
/*
two messages are sent so fast that the server reads two messages in one go
    therefore dropping a message or erroring out

maybe I need to
*/