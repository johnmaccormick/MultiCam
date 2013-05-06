// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
#pragma once

#define WINDOW_CLASS_NAME_LENGTH 128

bool Initialize_CreateWindowClass(WNDPROC wndProc, 
		char* windowClassNamePrefix,
		/*out*/ HINSTANCE &hInit_ProcessHandle,  
		/*out*/ char acInit_WindowClassName[128]);
void DeInitialize_DestroyWindowClass(char* acInit_WindowClassName, /*inout*/ HINSTANCE &hInit_ProcessHandle);
bool Initialize_CreateMainWindow(char* acInit_WindowClassName, HINSTANCE hInit_ProcessHandle,
			void* pThis, /*out*/ HWND &hInit_MainWindowHandle);
void DeInitialize_DestroyMainWindow(/*inout*/ HWND &hInit_MainWindowHandle);


