// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
#include <windows.h>
#include <strsafe.h>
#include "MessageBox.h"

// Example of how to convert your code string to ASCII before calling Msg:
// (needs stdlib.h)

	//WCHAR* jmu = L"asdf";
	//CHAR jma[MSG_STRING_MAXLEN];
	//size_t num_chars_converted;
	//wcstombs_s(&num_chars_converted, (CHAR*) jma, MSG_STRING_MAXLEN, jmu, _TRUNCATE);

	//Msg("Finished constructing EZrgb24 filter (%s)", jma);

void Msg(CHAR *szFormat, ...)
{
    TCHAR szBuffer[MSG_STRING_MAXLEN];  // Large buffer for long filenames or URLs
    const size_t NUMCHARS = sizeof(szBuffer) / sizeof(szBuffer[0]);
    const int LASTCHAR = NUMCHARS - 1;

    // Format the input string
    va_list pArgs;
    va_start(pArgs, szFormat);

    // Use a bounded buffer size to prevent buffer overruns.  Limit count to
    // character size minus one to allow for a NULL terminating character.
    (void)StringCchVPrintf(szBuffer, NUMCHARS - 1, szFormat, pArgs);
    va_end(pArgs);

    // Ensure that the formatted string is NULL-terminated
    szBuffer[LASTCHAR] = TEXT('\0');

    MessageBox(NULL, szBuffer, TEXT("MultiCam Filter Message"), MB_OK | MB_ICONERROR);
}