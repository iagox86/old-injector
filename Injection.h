
#include <windows.h>
#include <WindowsX.h>
#include <tchar.h>
#include <malloc.h>
#include <TlHelp32.h>

#ifdef UNICODE
#define InjectLib InjectLibW
#define EjectLib  EjectLibW
#else // if(!UNICODE)
#define InjectLib InjectLibA
#define EjectLib EjectLibA
#endif // !UNICODE


BOOL WINAPI InjectLibW(HANDLE hProcess, PCWSTR pszLibFile)
{
	BOOL fOk = FALSE; //Assume that the function fails
	HANDLE hThread = NULL;
	PWSTR pszLibFileRemote = NULL;

	__try 
	{
		// Calculate the number of bytes needed fot he dll's pathname
		int cch = 1+lstrlenW(pszLibFile);
		int cb = cch * sizeof(WCHAR);

		// Allocate space in the remote process for the pathname:
		pszLibFileRemote = (PWSTR) VirtualAllocEx(hProcess, NULL, cb, MEM_COMMIT, PAGE_READWRITE);
		if (pszLibFileRemote == NULL) __leave;

		// Copy the DLL's pathname to the remote process's address space
		if (!WriteProcessMemory(hProcess, pszLibFileRemote, (PVOID) pszLibFile, cb, NULL)) __leave;

		// Get the real address of LoadLibraryW in Kernel32.dll
		PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE) GetProcAddress(
			GetModuleHandle(TEXT("Kernel32")), "LoadLibraryW");
		if (pfnThreadRtn == NULL) __leave;

		// Create a remote thread that calls LoadLibraryW(DLLPathname)
		hThread = CreateRemoteThread(hProcess, NULL, 0, pfnThreadRtn, pszLibFileRemote, 0, NULL);
		if(hThread == NULL) __leave;

		// Wait for the remote thread to terminate
		WaitForSingleObject(hThread, INFINITE);

		fOk = TRUE;
	}
	__finally
	{
		// Free the remote memory that continaed the DLL's pathname
		if(pszLibFileRemote != NULL)
			VirtualFreeEx(hProcess, pszLibFileRemote, 0, MEM_RELEASE);

	}

	return fOk;
}

BOOL WINAPI InjectLibA(HANDLE hProcess, PCSTR pszLibFile)
{
	//** Convert the string to Unicode:
	// Allocate enough memory for the pathname:
	PWSTR pszLibFileW = (PWSTR) _alloca((lstrlenA(pszLibFile)+1) * sizeof(WCHAR));
	// Convert the ANSI pathname to Unicode
	wsprintfW(pszLibFileW, L"%S", pszLibFile);

	return(InjectLibW(hProcess, pszLibFileW));
}

BOOL WINAPI EjectLibW(HANDLE hProcess, DWORD dwProcessId, PCWSTR pszLibFile)
{
	BOOL fOk = FALSE; // Assume that the function fails
	HANDLE hthSnapshot = NULL;
	HANDLE hThread = NULL;

	__try {
		// Grab a snapshot of the process
		hthSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
		if(hthSnapshot == NULL) __leave;

		// Get the HMODULE of the desired library
		MODULEENTRY32W me = { sizeof(me) };
		BOOL fFound = FALSE;
		BOOL fMoreMods = Module32FirstW(hthSnapshot, &me);
		for(; fMoreMods; fMoreMods = Module32NextW(hthSnapshot, &me))
		{
			fFound = (lstrcmpiW(me.szModule, pszLibFile) == 0) || (lstrcmpiW(me.szExePath, pszLibFile) == 0);
			if(fFound) break;
		}

		if(!fFound) __leave;

		// Get the real address of LoadLibraryW in Kernel32.dll
		PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)
			GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "FreeLibrary");
		if (pfnThreadRtn == NULL) __leave;

		// Create a remote thread that calls LoadLibraryW(DLLPathname)
		hThread = CreateRemoteThread(hProcess, NULL, 0, pfnThreadRtn, me.modBaseAddr, 0, NULL);
		if(hThread == NULL) __leave;

		// Wait for the remove thread to terminate
		WaitForSingleObject(hThread, INFINITE);

		fOk = TRUE; // Everything executed successfully
	}
	__finally
	{
		//Clean everything up
		if(hthSnapshot != NULL)
		{
			CloseHandle(hthSnapshot);
		}

	}

	return fOk;
}

BOOL WINAPI EjectLibA(HANDLE hProcess, DWORD dwProcessId, PCSTR pszLibFile)
{
	// Allocate a (stack) buffer for the unicode version of the pathname
	PWSTR pszLibFileW = (PWSTR)
		_alloca((lstrlenA(pszLibFile)+1) * sizeof(WCHAR));

	// Convert the ANSI pathname to unicode
	wsprintfW(pszLibFileW, L"%S", pszLibFile);

	// Call the unicode version of this function
	return(EjectLibW(hProcess, dwProcessId, pszLibFileW));
}
