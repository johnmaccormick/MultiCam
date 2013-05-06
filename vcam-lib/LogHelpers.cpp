// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
#include <streams.h>
#include <stdio.h>
#include <time.h>

#include <iostream>
#include <fstream>

#include <strsafe.h>

using namespace std;

#include "win32_exception.h"
#include "jmac-vcam-guids.h"
#include "MessageBox.h"
#include "LogHelpers.h"
#include "ConnectHelpers.h"



//  !!!! Must be top-level directory !!!!!
const char *VCAM_DEBUG_DIRNAME = "C:\\temp";
//  !!!! Directory must agree with VCAM_DEBUG_DIRNAME !!!!!!
const char *VCAM_DEBUG_FNAME = "C:\\temp\\vcam-log.txt";

void __stdcall vcamLogInternal(int verbosity, char* message);
void __stdcall vcamLogVarargs(int verbosity, CHAR *szFormat, va_list pArgs);
void __stdcall vcamLogInternal(int verbosity, ofstream &out, char* message);
void __stdcall vcamLogVarargs(int verbosity, ofstream &out, TCHAR *szFormat, va_list pArgs);
void __stdcall vcamLogFormat(int verbosity, ofstream& out, const AM_MEDIA_TYPE *pmt);


static ofstream debugOut;
static int numUsers;
static const int verbosityLevel = 100;
static BOOL openedOK = FALSE;
static BOOL showedLogWarning = FALSE;
static CCritSec debugLock;

void __stdcall vcamOpenLog(int verbosity, char* message)
{
#ifdef _DEBUG
	{
		CAutoLock l(&debugLock);
		if (numUsers == 0) {
			BOOL ok = CreateDirectory(VCAM_DEBUG_DIRNAME, NULL);
			if (!ok && GetLastError() == ERROR_ALREADY_EXISTS)
			{
				ok = TRUE;
			}
			if (ok) {
				try {
					debugOut.exceptions(ios::failbit);
					debugOut.open(VCAM_DEBUG_FNAME, ios_base::app);
					// Truncate log if it's too big
					debugOut.seekp(0, ios_base::end);
					if (debugOut.tellp() > 1024 * 256) {
						debugOut.close();
						debugOut.open(VCAM_DEBUG_FNAME, ios_base::trunc);
					}
					openedOK = TRUE;
				}
				catch (...)
				{
					openedOK = FALSE;
				}
			}
			else 
			{
				openedOK = FALSE;
			}
			if (!openedOK && !showedLogWarning)
			{
				Msg("Couldn't open multicam log file,\r\nso no logging will be performed");
				showedLogWarning = TRUE;
			}
		}
		numUsers++;
	}
	vcamLog(verbosity, "%s opened log, numUsers = %d", message, numUsers);
#endif
}

void __stdcall vcamCloseLog(int verbosity, char* message)
{
#ifdef _DEBUG
	vcamLog(verbosity, "%s closing log, numUsers = %d", message, numUsers);
	{
		CAutoLock l(&debugLock);
		numUsers--;
		if (numUsers == 0 && openedOK) {
			debugOut.close();
		}
	}
#endif
}

void __stdcall vcamLogInternal(int verbosity, char* message)
{
#ifdef _DEBUG
	vcamLogInternal(verbosity, debugOut, message);
#endif
}

void __stdcall vcamLogVarargs(int verbosity, TCHAR *szFormat, va_list pArgs)
{
#ifdef _DEBUG
	vcamLogVarargs(verbosity, debugOut, szFormat, pArgs);
#endif
}

void __stdcall vcamLogInternal(int verbosity, ofstream &out, char* message)
{
#ifdef _DEBUG
	SYSTEMTIME t;
	GetLocalTime(&t);

	CAutoLock l(&debugLock);
	if (verbosity <= verbosityLevel)
	{
		if ( numUsers <= 0 ) 
		{
			Msg(message);
		}
		if (openedOK) 
		{
			out 
				<< t.wMonth << "/" << t.wDay << "/" << t.wYear << ","
				<< t.wHour << ":" << t.wMinute << ":" << t.wSecond << "." << t.wMilliseconds << ": " 
				<< message << endl;
		}
	}
#endif
}

void __stdcall vcamLogVarargs(int verbosity, ofstream &out, TCHAR *szFormat, va_list pArgs)
{
#ifdef _DEBUG
	TCHAR szBuffer[1024];  // Large buffer for long filenames or URLs
	const size_t NUMCHARS = sizeof(szBuffer) / sizeof(szBuffer[0]);
	const int LASTCHAR = NUMCHARS - 1;

	//// Format the input string
	//va_list pArgs;
	//va_start(pArgs, szFormat);

	// Use a bounded buffer size to prevent buffer overruns.  Limit count to
	// character size minus one to allow for a NULL terminating character.
	HRESULT hr = StringCchVPrintfA(szBuffer, NUMCHARS - 1, szFormat, pArgs);
	if(!SUCCEEDED(hr)) return;
	//va_end(pArgs);

	// Ensure that the formatted string is NULL-terminated
	szBuffer[LASTCHAR] = '\0';

	vcamLogInternal(verbosity, out, szBuffer);
	//CAutoLock l(&debugLock);
	//VCAM_ASSERT(numUsers > 0);
	//out << szBuffer << endl;
#endif
}

void __stdcall vcamLog(int verbosity, ofstream& out, TCHAR *szFormat, ...)
{
#ifdef _DEBUG
	va_list pArgs;
	va_start(pArgs, szFormat);
	vcamLogVarargs(verbosity, out, szFormat, pArgs);
	va_end(pArgs);
#endif
}

void __stdcall vcamLog(int verbosity, TCHAR *szFormat, ...)
{
#ifdef _DEBUG
	va_list pArgs;
	va_start(pArgs, szFormat);
	vcamLogVarargs(verbosity, szFormat, pArgs);
	va_end(pArgs);
#endif
}

void __stdcall vcamLogFormat(int verbosity, const AM_MEDIA_TYPE *pmt)
{
#ifdef _DEBUG
	vcamLogFormat(verbosity, debugOut, pmt);
#endif
}

void __stdcall vcamLogFormat(int verbosity, ofstream& out, const AM_MEDIA_TYPE *pmt)
{
#ifdef _DEBUG
	if (pmt == NULL) {
		vcamLog(verbosity, "NULL");
		return;
	}

	if (pmt->majortype != MEDIATYPE_Video)
	{
		vcamLog(verbosity, out, "ERROR: vcamLogFormat encountered a non-video media type");
		return;
	}

	TCHAR* subtypeName = GetSubtypeName(&(pmt->subtype));
	vcamLog(verbosity, out, "subtypeName = %s", subtypeName);

	char guidString[GUID_STRING_MAXLEN];
	Riid2String(pmt->subtype, guidString);
	vcamLog(verbosity, out, "subtype GUID = %s", guidString);

	vcamLog(verbosity, out, "bFixedSizeSamples = %d", pmt->bFixedSizeSamples);
	vcamLog(verbosity, out, "bTemporalCompression = %d", pmt->bTemporalCompression);
	vcamLog(verbosity, out, "lSampleSize = %lu", pmt->lSampleSize);

	if(pmt->formattype == FORMAT_VideoInfo || pmt->formattype == FORMAT_VideoInfo2) {
		vcamLog(verbosity, out, "VIDEOINFOHEADER:");
		VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*) pmt->pbFormat;
		RECT r = pvi->rcSource;
		vcamLog(verbosity, out, "rcSource = %d, %d, %d, %d", r.left, r.top, r.bottom, r.right);
		r = pvi->rcTarget;
		vcamLog(verbosity, out, "rcTarget = %d, %d, %d, %d", r.left, r.top, r.bottom, r.right);
		vcamLog(verbosity, out, "dwBitRate = %d", pvi->dwBitRate);
		vcamLog(verbosity, out, "dwBitErrorRate = %d", pvi->dwBitErrorRate);
		vcamLog(verbosity, out, "AvgTimePerFrame = %ld", pvi->AvgTimePerFrame);
		BITMAPINFOHEADER* bmi = GetBITMAPINFOHEADER(pmt);
		vcamLog(verbosity, out, "BITMAPINFOHEADER:");
		vcamLog(verbosity, out, "biSize = %d", bmi->biSize);
		vcamLog(verbosity, out, "biWidth = %ld", bmi->biWidth);
		vcamLog(verbosity, out, "biHeight = %ld", bmi->biHeight);
		vcamLog(verbosity, out, "biPlanes = %hd", bmi->biPlanes);
		vcamLog(verbosity, out, "biBitCount = %hd", bmi->biBitCount);
		vcamLog(verbosity, out, "biCompression = %d", bmi->biCompression);
		vcamLog(verbosity, out, "biSizeImage = %d", bmi->biSizeImage);
		vcamLog(verbosity, out, "biXPelsPerMeter = %ld", bmi->biXPelsPerMeter);
		vcamLog(verbosity, out, "biYPelsPerMeter = %ld", bmi->biYPelsPerMeter);
		vcamLog(verbosity, out, "biClrUsed = %d", bmi->biClrUsed);
		vcamLog(verbosity, out, "biClrImportant = %d", bmi->biClrImportant);
	}
	else {
		vcamLog(verbosity, out, "unknown formattype");
	}



#endif
}

void __stdcall vcamLogError(int verbosity,  TCHAR* msg )
{
#ifdef _DEBUG
	DWORD eNum;
	TCHAR sysMsg[256];
	TCHAR* p;

	eNum = GetLastError( );
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, eNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		sysMsg, 256, NULL );

	// Trim the end of the line and terminate it with a null
	p = sysMsg;
	while( ( *p > 31 ) || ( *p == 9 ) )
		++p;
	do { *p-- = 0; } while( ( p >= sysMsg ) &&
		( ( *p == '.' ) || ( *p < 33 ) ) );

	// Display the message
	vcamLog(verbosity,  TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg );
#endif
}

void __stdcall vcamLogException(const win32_exception& e)
{
#ifdef _DEBUG
	vcamLog(0, "EXCEPTION: %s (code 0x%x) at 0x%x in %s", 
		e.what(), e.code(), e.where(), __FILE__);
#endif
}

void __stdcall vcamLogException(const win32_exception& e, const char* file, const int line)
{
#ifdef _DEBUG
	vcamLog(0, "EXCEPTION: %s (code 0x%x) at 0x%x in %s at line %d", 
		e.what(), e.code(), e.where(), file, line);
#endif
}

