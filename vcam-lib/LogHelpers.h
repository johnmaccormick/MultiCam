// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
#pragma once

#include "win32_exception.h"
#include "MessageBox.h"

void __stdcall vcamOpenLog(int verbosity, char* message);
void __stdcall vcamCloseLog(int verbosity, char* message);
void __stdcall vcamLog(int verbosity, CHAR *szFormat, ...);
void __stdcall vcamLogFormat(int verbosity, const AM_MEDIA_TYPE *pmt);
void __stdcall vcamLogError(int verbosity,  TCHAR* msg );
void __stdcall vcamLogException(const win32_exception& e );
void __stdcall vcamLogException(const win32_exception& e, const char* file, 
	const int line);

#ifdef _DEBUG

#define VCAM_TRY try {

#define VCAM_CATCH } \
	catch (const win32_exception& e) { \
		vcamLogException(e, __FILE__, __LINE__); \
		Msg("MultiCam encountered an unexpected exception.\r\nPlease see the log file for more details."); \
		throw; \
    }

#define VCAM_ASSERT(_x_) if (!(_x_)) {        \
            vcamLog(0, "ASSERTION FAILURE of '%s' in %s at %d", TEXT(#_x_),TEXT(__FILE__),__LINE__); \
			Msg("MultiCam encountered an assertion failure.\r\nPlease see the log file for more details."); }

#define VCAM_ASSERT_NOBOX(_x_) if (!(_x_)) {        \
            vcamLog(0, "ASSERTION FAILURE of '%s' in %s at %d", TEXT(#_x_),TEXT(__FILE__),__LINE__);}

#else
#define VCAM_TRY 
#define VCAM_CATCH 
#define VCAM_ASSERT(_x_) 
#define VCAM_ASSERT_NOBOX(_x_)
#endif



