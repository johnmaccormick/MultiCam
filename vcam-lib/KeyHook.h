// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
BOOL GetPIDFromExeName(TCHAR* szExeFile, /*out*/ DWORD* pPid);
BOOL AddHookToThreadIDs(vector<DWORD> &tids);
BOOL AddHookToThreadID(DWORD tid);
BOOL GetProcessThreadIDs(TCHAR* szExeFile, /*out*/ vector<DWORD> &tids);
BOOL GetProcessThreadIDs(DWORD dwOwnerPID, /*out*/ vector<DWORD> &tids);


