// Copied and altered from http://msdn.microsoft.com/en-us/library/aa365588(v=VS.85).aspx
// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.

#include <streams.h>

#include <iostream>
#include <fstream>

#include <windows.h> 
#include <stdio.h> 
#include <tchar.h>
#include <strsafe.h>
#include <assert.h>

using namespace std;

#include "jmac-vcam-guids.h"
#include "LogHelpers.h"

#define BUFSIZE 512

DWORD WINAPI InstanceThread(LPVOID); 
VOID GetAnswerToRequest(LPTSTR, LPTSTR, LPDWORD); 
DWORD WINAPI RunPipeServer(LPVOID lpvParam);

extern HWND g_hMainWindow;

typedef struct Pipe_Info {
	char name[MAX_PATH];
} PIPE_INFO, *P_PIPE_INFO;

BOOL RunPipeServerInThread(char* pipeName)
{
	HANDLE hThread = NULL; 

	P_PIPE_INFO pPipeInfo = (P_PIPE_INFO) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PIPE_INFO));
	VCAM_ASSERT(pPipeInfo != NULL);

	strcpy_s(pPipeInfo->name, pipeName);

	hThread = CreateThread( 
		NULL,              // no security attribute 
		0,                 // default stack size 
		RunPipeServer,    // thread proc
		(LPVOID) &pPipeInfo,    // thread parameter 
		0,                 // not suspended 
		NULL);      // returns thread ID 

	if (hThread == NULL) 
	{
		HeapFree(GetProcessHeap(), 0, pPipeInfo);
		vcamLogError(10, TEXT("RunPipeServerInThread: CreateThread failed")); 
		return FALSE;
	}
	else CloseHandle(hThread);

	return TRUE;
}

DWORD WINAPI RunPipeServer(LPVOID lpvParam)
{ 
	BOOL   fConnected = FALSE; 
	DWORD  dwThreadId = 0; 
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = NULL; 
	P_PIPE_INFO pPipeInfo = (P_PIPE_INFO) lpvParam;
	char pipeName[MAX_PATH];
	strcpy_s(pipeName, pPipeInfo->name);
	HeapFree(GetProcessHeap(), 0, pPipeInfo);
	
	vcamOpenLog(5, "RunPipeServer");

	// The main loop creates an instance of the named pipe and 
	// then waits for a client to connect to it. When the client 
	// connects, a thread is created to handle communications 
	// with that client, and this loop is free to wait for the
	// next client connect request. It is an infinite loop.

	for (;;) 
	{ 
		vcamLog(10,  TEXT("\nRunPipeServer: Pipe Server: Main thread awaiting client connection on %s\n"), pipeName);
		hPipe = CreateNamedPipe( 
			pipeName,             // pipe name 
			PIPE_ACCESS_DUPLEX,      // 
			PIPE_TYPE_BYTE |       // message type pipe 
			PIPE_READMODE_BYTE |   // message-read mode 
			PIPE_WAIT,                // blocking mode 
			PIPE_UNLIMITED_INSTANCES, // max. instances  
			BUFSIZE,                  // output buffer size 
			BUFSIZE,                  // input buffer size 
			0,                        // client time-out 
			NULL);                    // default security attribute 

		if (hPipe == INVALID_HANDLE_VALUE) 
		{
			vcamLog(10, TEXT("RunPipeServer: CreateNamedPipe failed, GLE=%d.\n"), GetLastError()); 
			return -1;
		}

		// Wait for the client to connect; if it succeeds, 
		// the function returns a nonzero value. If the function
		// returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 

		fConnected = ConnectNamedPipe(hPipe, NULL) ? 
TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 

		if (fConnected) 
		{ 
			vcamLog(10, "RunPipeServer: Client connected, creating a processing thread."); 

			// Create a thread for this client. 
			hThread = CreateThread( 
				NULL,              // no security attribute 
				0,                 // default stack size 
				InstanceThread,    // thread proc
				(LPVOID) hPipe,    // thread parameter 
				0,                 // not suspended 
				&dwThreadId);      // returns thread ID 

			if (hThread == NULL) 
			{
				vcamLog(10, TEXT("RunPipeServer: CreateThread failed, GLE=%d."), GetLastError()); 
				return -1;
			}
			else CloseHandle(hThread); 
		} 
		else 
			// The client could not connect, so close the pipe. 
			CloseHandle(hPipe); 
	} 

	vcamCloseLog(5, "RunPipeServer");
	return 0; 
} 

DWORD WINAPI InstanceThread(LPVOID lpvParam)
	// This routine is a thread processing function to read from and reply to a client
	// via the open pipe connection passed from the main loop. Note this allows
	// the main loop to continue executing, potentially creating more threads of
	// of this procedure to run concurrently, depending on the number of incoming
	// client connections.
{ 
	HANDLE hHeap      = GetProcessHeap();
	DWORD cbReplyBytes = 0, cbWritten = 0; 
	BOOL fSuccess = FALSE;
	HANDLE hPipe  = NULL;

	// Do some extra error checking since the app will keep running even if this
	// thread fails.

	if (lpvParam == NULL)
	{
		vcamLog(10,  "ERROR - Pipe Server Failure:");
		vcamLog(10,  "   InstanceThread got an unexpected NULL value in lpvParam.");
		vcamLog(10,  "   InstanceThread exitting.");
		return (DWORD)-1;
	}

	// Print verbose messages. In production code, this should be for debugging only.
	vcamLog(10, "InstanceThread created, receiving and processing messages.");

	// The thread's parameter is a handle to a pipe object instance. 

	hPipe = (HANDLE) lpvParam; 

	// Write the reply to the pipe. 
	cbReplyBytes = sizeof(g_hMainWindow);
	fSuccess = WriteFile( 
		hPipe,        // handle to pipe 
		&g_hMainWindow,     // buffer to write from 
		cbReplyBytes, // number of bytes to write 
		&cbWritten,   // number of bytes written 
		NULL);        // not overlapped I/O 

	if (!fSuccess || cbReplyBytes != cbWritten)
	{   
		vcamLog(10, TEXT("InstanceThread WriteFile failed, GLE=%d.\n"), GetLastError()); 
		vcamLogError(10, "InstanceThread: WriteFile");
	}


	// Flush the pipe to allow the client to read the pipe's contents 
	// before disconnecting. Then disconnect the pipe, and close the 
	// handle to this pipe instance. 

	FlushFileBuffers(hPipe); 
	DisconnectNamedPipe(hPipe); 
	CloseHandle(hPipe); 

	vcamLog(10, "InstanceThread exiting.");
	return 1;
}


DWORD WINAPI RunPipeClient(LPVOID lpvParam)
{ 
	HANDLE hPipe; 
	LPTSTR lpvMessage=TEXT("Default message from client."); 
	TCHAR  chBuf[BUFSIZE]; 
	BOOL   fSuccess = FALSE; 
	DWORD  cbRead, dwMode; 
	LPTSTR lpszPipename = /*JmacKeyHookPipeName*/ ""; // temp change

	// Try to open a named pipe; wait for it, if necessary. 

	while (1) 
	{ 
		hPipe = CreateFile( 
			lpszPipename,   // pipe name 
			//GENERIC_READ, 
			GENERIC_READ |  // read and write access 
         GENERIC_WRITE, 

			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

		// Break if the pipe handle is valid. 

		if (hPipe != INVALID_HANDLE_VALUE) 
			break; 

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 

		if (GetLastError() != ERROR_PIPE_BUSY) 
		{
			vcamLog(10,  TEXT("RunPipeClient: Could not open pipe. GLE=%d\n"), GetLastError() ); 
			vcamLogError(10, "RunPipeClient");
			return -1;
		}

		// All pipe instances are busy, so wait for 20 seconds. 
		vcamLog(10, "Could not open pipe: will wait 20 seconds."); 
		if ( ! WaitNamedPipe(lpszPipename, 20000)) 
		{ 
			vcamLog(10, "Could not open pipe: 20 second wait timed out."); 
			return -1;
		} 
	} 

	// The pipe connected; change to message-read mode. 

	dwMode = PIPE_READMODE_BYTE; 
	fSuccess = SetNamedPipeHandleState( 
		hPipe,    // pipe handle 
		&dwMode,  // new pipe mode 
		NULL,     // don't set maximum bytes 
		NULL);    // don't set maximum time 
	if ( ! fSuccess) 
	{
		vcamLog(10,  TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError() ); 
		vcamLogError(10, "RunPipeClientInThread: SetNamedPipeHandleState");

		return -1;
	}


	do 
	{ 
		// Read from the pipe. 

		fSuccess = ReadFile( 
			hPipe,    // pipe handle 
			chBuf,    // buffer to receive reply 
			BUFSIZE*sizeof(TCHAR),  // size of buffer 
			&cbRead,  // number of bytes read 
			NULL);    // not overlapped 

		if ( ! fSuccess && GetLastError() != ERROR_MORE_DATA )
			break; 

		vcamLog(10,  TEXT("\"%s\"\n"), chBuf ); 
	} while ( ! fSuccess);  // repeat loop if ERROR_MORE_DATA 

	if ( ! fSuccess)
	{
		vcamLog(10,  TEXT("ReadFile from pipe failed. GLE=%d\n"), GetLastError() );
		vcamLogError(10, "RunPipeClientInThread: ReadFile");
		return -1;
	}

	assert(cbRead == sizeof(HWND));
	//hwJmacKeyHookWindow =  *((HWND*)chBuf); // temporary change
	//vcamLog(10, "RunPipeClientInThread: got handle hwJmacKeyHookWindow = 0x%x", hwJmacKeyHookWindow);

	CloseHandle(hPipe); 

	return 0; 
}