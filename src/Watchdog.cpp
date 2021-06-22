#include <windows.h> 
#include <stdio.h> 
#include <tchar.h>
#include <strsafe.h>
#include <iostream>
#include <aclapi.h>

#include "Watchdog.h"

#define BUFSIZE 512

namespace WatchDog
{

    DWORD WINAPI WDInstanceThread(LPVOID param)
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
            printf("Heap allocation for security descriptor failed. GLE=%u\n", GetLastError());
            return 1;
        }

        // The InitializeSecurityDescriptor function initializes a security descriptor to have no system
        // access control list(SACL), no discretionary access control list(DACL), no owner, no primary group,
        // and all control flags set to FALSE(NULL). Thus, except for its revision level, it is empty
        if (NULL == InitializeSecurityDescriptor(pSD.get(), SECURITY_DESCRIPTOR_REVISION))
        {
            printf("InitializeSecurityDescriptor Error. GLE=%u\n", GetLastError());
            //HeapFree(hHeap, 0, pSD);
            return 1;
        }


        // Add the Access Control List (ACL) to the security descriptor. 
//Allow all access to the pipe
#pragma warning(disable:6248)
        if (!SetSecurityDescriptorDacl(
            pSD.get(),
            TRUE,        // bDaclPresent flag   
            (PACL)NULL,  // if a NULL DACL is assigned to the security descriptor, all access is allowed
            FALSE))      // not a default DACL 
        {
            printf("SetSecurityDescriptorDacl Error %u\n", GetLastError());
            //goto Cleanup;
        }
#pragma warning(default:4700)

        // Initialize a security attributes structure.
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = pSD.get();
        sa.bInheritHandle = FALSE;

        printf("\nPipe Server: Main thread awaiting client connection on %s\n", lpszPipename);

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
            printf("CreateNamedPipe failed, GLE=%d.\n", GetLastError());
            return -1;
        }

        result = SetEvent(*phEvent);
        lala = WDServe(hPipe);

        //HeapFree(hHeap, 0, pSD);
        return 0;
    }

    int WDServe(HANDLE hPipe)
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
            printf("\nERROR - Pipe Server Failure:\n");
            printf("   Serve got an unexpected NULL heap allocation.\n");
            printf("   Serve exitting.\n");
            if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
            return (DWORD)-1;
        }

        if (pchReply == NULL)
        {
            printf("\nERROR - Pipe Server Failure:\n");
            printf("   Serve got an unexpected NULL heap allocation.\n");
            printf("   Serve exitting.\n");
            if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
            return (DWORD)-1;
        }

        while (1)
        {
            printf("Waiting on client connection...\n");

            if (ConnectNamedPipe(hPipe, NULL) == 0)
            {
                printf("ConnectNamedPipe failed, GLE=%d.\n", GetLastError());
                break;
            }
            printf("Client Connected!\n");

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
                        printf("Serve: client disconnected.\n");
                        break;
                    }
                    else
                    {
                        printf("WDInstanceThread ReadFile failed, GLE=%d.\n", GetLastError());
                        break;
                    }
                }

                //printf("CLIENT MSG: %s\n", pchRequest);

                // Process the incoming message.
                WDHandleMsg(pchRequest, pchReply, &cbReplyBytes);

                //_tcscpy_s(pchReply, BUFSIZE, TEXT("ACK"));
                //cbReplyBytes = BUFSIZE * sizeof(char);
                Sleep(100);

                // Write the reply to the pipe. 

                bSuccess = WriteFile(
                    hPipe,        // handle to pipe 
                    pchReply,     // buffer to write from
                    //pchRequest,
                    cbReplyBytes, // number of bytes to write 
                    &cbWritten,   // number of bytes written 
                    NULL);        // not overlapped I/O 

                if (!bSuccess || cbReplyBytes != cbWritten)
                {
                    printf("WDInstanceThread WriteFile failed, GLE=%d.\n", GetLastError());
                }
                else
                {
                    printf("Number of Bytes Written: %d\n", cbReplyBytes);
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

        printf("WDInstanceThread exiting.\n");
        return (DWORD)1;
    }

    VOID WDHandleMsg(const char* pchRequest, char* pchReply, LPDWORD pchBytes)
        // Reads message and performs actions based on the content
        // Writes a reply into the reply char*
    {
        const char* msg_ack = "ACK";
        errno_t rslt = 1;

        printf("Client Request:\n%s\n", pchRequest);

        if (strcmp(pchRequest, "KILL") == 0)
        {
            rslt = strcpy_s(pchReply, BUFSIZE, "KL");
            int retval = system("taskkill /T /IM TrackIR5.exe");
        }
        else if (strcmp(pchRequest, "HEARTBEAT") == 0)
        {
            rslt = strcpy_s(pchReply, BUFSIZE, "HB");
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
            printf("strcpy_s failed, no outgoing message.\n");
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

