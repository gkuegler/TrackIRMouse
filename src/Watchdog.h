#include <windows.h> 
#include <stdio.h> 
#include <tchar.h>
#include <strsafe.h>
#include <iostream>
#include <aclapi.h>

#define BUFSIZE 512

namespace WatchDog {


    DWORD WINAPI WDInstanceThread(LPVOID);
    int WDServe(HANDLE);
    void WDHandleMsg(const char*, char*, LPDWORD);

    DWORD WINAPI WDInstanceThread(LPVOID inum)
    {
        //HANDLE hPipe = INVALID_HANDLE_VALUE;
        HANDLE hHeap = GetProcessHeap();
        HANDLE hPipe = (HANDLE)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(HANDLE));
        LPCSTR lpszPipename = "\\\\.\\pipe\\watchdog";
        DWORD dwServeRslt = -1;
        int lala = 0;


        // The main loop creates an instance of the named pipe and 
        // then waits for a client to connect to it. When the client 
        // connects, a thread is created to handle communications 
        // with that client, and this loop is free to wait for the
        // next client connect request. It is an infinite loop.

        PSID pEveryoneSID = NULL, pAdminSID = NULL;
        PACL pACL = NULL;
        PSECURITY_DESCRIPTOR pSD = NULL;
        EXPLICIT_ACCESS ea[2];
        SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
        SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
        SECURITY_ATTRIBUTES sa;
        HKEY hkSub = NULL;

        ZeroMemory(&ea, 2 * sizeof(EXPLICIT_ACCESS));
        ea[0].grfAccessPermissions = KEY_READ;
        ea[0].grfAccessMode = SET_ACCESS;
        ea[0].grfInheritance = NO_INHERITANCE;
        ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea[0].Trustee.ptstrName = (LPTSTR)pEveryoneSID;

        // Create a SID for the BUILTIN\Administrators group.
        if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &pAdminSID))
        {
            printf("AllocateAndInitializeSid Error %u\n", GetLastError());
            goto Cleanup;
        }

        // Initialize an EXPLICIT_ACCESS structure for an ACE.
        // The ACE will allow the Administrators group full access to
        // the key.
        ea[1].grfAccessPermissions = KEY_ALL_ACCESS;
        ea[1].grfAccessMode = SET_ACCESS;
        ea[1].grfInheritance = NO_INHERITANCE;
        ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        ea[1].Trustee.ptstrName = (LPTSTR)pAdminSID;

        // Initialize a security descriptor.
        //don't have a DACL, will grant access to everyone
        //pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
        pSD = (PSECURITY_DESCRIPTOR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, SECURITY_DESCRIPTOR_MIN_LENGTH);

        if (NULL == pSD)
        {
            printf("Security Descriptor Alloc Error %u\n", GetLastError());
            goto Cleanup;
        }

        if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
        {
            printf("InitializeSecurityDescriptor Error %u\n", GetLastError());
            goto Cleanup;
        }

        // Add the ACL to the security descriptor. 
#pragma warning(disable:6248)
        if (!SetSecurityDescriptorDacl(
            pSD,
            TRUE,     // bDaclPresent flag   
            (PACL)NULL,
            FALSE))    // not a default DACL 
        {
            printf("SetSecurityDescriptorDacl Error %u\n", GetLastError());
            goto Cleanup;
        }
#pragma warning(default:4700)

        // Initialize a security attributes structure.
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = pSD;
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
        lala = WDServe(hPipe);

    Cleanup:
        HeapFree(hHeap, 0, pSD);
        return 0;
    }

    int WDServe(HANDLE hPipe)
    {
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