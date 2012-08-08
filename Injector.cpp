// Injector.cpp : Defines the entry point for the application.
//


#include "stdafx.h"
#include "Injector.h"
#include "Injection.h"
#define MAX_LOADSTRING 100

#include <stdio.h>
#include <stdlib.h>
// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:

LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
HANDLE getHandle(HWND hDlg, char *pName, DWORD *retPid);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     char*     lpCmdLine,
                     int       nCmdShow)
{
	// If there's a paramter, then we're going to attempt to parse it and 
	// if possible inject or eject the dll
	if(*lpCmdLine)
	{
		// Expected format is "injector [ie]" to inject or eject the most recent
		// or injector [ie] "Window" "Dll"
		unsigned char Process[512];
		unsigned char Dll[512];
		bool inject;
		if(tolower(*lpCmdLine) == 'i')
		{
			inject = true;
		}
		else if(tolower(*lpCmdLine) == 'e')
		{
			inject = false;
		}
		else
		{
			MessageBox(NULL, "Either specify no parameter for gui, or\n1) 'injector.exe [ie]' to inject or eject the most previous used (in gui), or\n2) 'injector.exe [ie] \"Window\" \"Dll\"'", "", 1);
			exit(1);
		}
		

		lpCmdLine++;
		if(*lpCmdLine)
		{
			// Get the window/dll from commandline
			char *window = lpCmdLine += 2;
			lpCmdLine = strchr(lpCmdLine, '"');
			*lpCmdLine = '\0';

			char *dll = lpCmdLine += 3;

			lpCmdLine = strchr(lpCmdLine, '"');
			*lpCmdLine = '\0';

			strcpy((char*)Process, window);
			strcpy((char*)Dll, dll);

		}
		else
		{
			// Get the last Process and DLL
			HKEY RegKey;
			DWORD Size;
			
			if(RegOpenKey(HKEY_CURRENT_USER, "software\\iago\\injector", &RegKey) == ERROR_SUCCESS)
			{
				Size = 512;
				RegQueryValueEx(RegKey, "lastprocess", NULL, NULL, (BYTE*)Process, &Size);
				Size = 512;
				RegQueryValueEx(RegKey, "lastdll", NULL, NULL, (BYTE*)Dll, &Size);
				
			}
			
		}

		HANDLE ProcessHandle = getHandle(NULL, (char*)Process, NULL);

		if(ProcessHandle)
		{
			if(inject)
			{
				if(InjectLib(ProcessHandle, (char*)Dll))
				{
					//MessageBox(NULL,"Dll successfully injected!", "", 0);
					exit(0);
				}
				else
				{
					//MessageBox(NULL,"There was an error injecting the dll", "", 0);
					exit(0xbad);
				}
			}
			else
			{
				DWORD pid;

				HANDLE ProcessHandle = getHandle(NULL, (char*)Process, &pid);

				if(ProcessHandle)
				{
					if(EjectLib(ProcessHandle, pid, (char*)Dll))
					{
						//MessageBox(NULL,"Dll successfully ejected!", "", 0);
						exit(0);
					}
					else
					{
						//MessageBox(NULL,"There was an error ejecting the dll", "", 0);
						exit(0);
					}
				}
			}
		}
		else
		{
			MessageBox(NULL, "Couldn't find window", "", 0);
			exit(0xbad);
		}


	}
	else
	{
		DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, NULL, (DLGPROC)About);
	}

	return 0;
}

// This gets a process's handle from its windows's name
HANDLE getHandle(HWND hDlg, char *pName, DWORD *retPid)
{
	if(pName == NULL || *pName == NULL)
	{
		SetDlgItemText(hDlg, ID_RESPONSE, "Error: You must supply a window name!");
		return NULL;
	}

	HWND window = FindWindow(NULL, pName);
	if(window == NULL)
	{
		SetDlgItemText(hDlg, ID_RESPONSE, "Error: Invalid window name!");
		return NULL;
	}
	
	DWORD pid = NULL;
	GetWindowThreadProcessId(window, &pid);
	if(pid == NULL)
	{
		SetDlgItemText(hDlg, ID_RESPONSE, "Error: Could not determine process id!");
		return NULL;
	}

	if(retPid)
		*retPid = pid;

	HANDLE ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	if(ProcessHandle == NULL)
	{
		SetDlgItemText(hDlg, ID_RESPONSE, "Error: Could not open process!");
		return NULL;
	}


	return ProcessHandle;
}

// Message handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			// Get the last Process and DLL
			HKEY RegKey;
			unsigned char RegData[512];
			DWORD Size;
			
			if(RegOpenKey(HKEY_CURRENT_USER, "software\\iago\\injector", &RegKey) == ERROR_SUCCESS)
			{
				Size = 512;
				*RegData = '\0';
				RegQueryValueEx(RegKey, "lastprocess", NULL, NULL, (BYTE*)RegData, &Size);
				SetDlgItemText(hDlg, IDPROCESS, (char*)RegData);
				Size = 512;
				*RegData = '\0';
				RegQueryValueEx(RegKey, "lastdll", NULL, NULL, (BYTE*)RegData, &Size);
				SetDlgItemText(hDlg, IDDLL, (char*)RegData);
			}
			else
			{
				SetDlgItemText(hDlg, IDPROCESS, "");
				SetDlgItemText(hDlg, IDDLL, "");
			}
			RegCloseKey(RegKey);


			SetDlgItemText(hDlg, ID_RESPONSE, "Ready.");

			return TRUE;
		}

	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL) 
		{
			// Save the new value to the registry
			HKEY RegKey;
			if(RegCreateKey(HKEY_CURRENT_USER, "software\\iago\\injector", &RegKey) == ERROR_SUCCESS)
			{
				char DLLName[512];
				char Process[512];
				GetDlgItemText(hDlg, IDDLL, DLLName, 511);
				GetDlgItemText(hDlg, IDPROCESS, Process, 511);
				RegSetValueEx(RegKey, "lastprocess", 0, REG_SZ, (BYTE*)Process, 512);
				RegSetValueEx(RegKey, "lastdll", 0, REG_SZ, (BYTE*)DLLName, 512);
			}
			EndDialog(hDlg, LOWORD(wParam));
		}
		else if(LOWORD(wParam) == IDINJECT)
		{
			char DLLName[256];
			char Process[256];
			GetDlgItemText(hDlg, IDDLL, DLLName, 255);
			GetDlgItemText(hDlg, IDPROCESS, Process, 255);

			HANDLE ProcessHandle = getHandle(hDlg, Process, NULL);

			if(ProcessHandle)
			{
				if(InjectLib(ProcessHandle, DLLName))
				{
					SetDlgItemText(hDlg, ID_RESPONSE, "Dll successfully injected!");
				}
				else
				{
					SetDlgItemText(hDlg, ID_RESPONSE, "There was an error injecting the dll");
				}
			}

			SetFocus(GetDlgItem(hDlg, IDPROCESS));
		}
		else if(LOWORD(wParam) == IDEJECT)
		{
			char DLLName[256];
			char Process[256];
			DWORD pid;
			GetDlgItemText(hDlg, IDDLL, DLLName, 255);
			GetDlgItemText(hDlg, IDPROCESS, Process, 255);

			HANDLE ProcessHandle = getHandle(hDlg, Process, &pid);

			if(hDlg, ProcessHandle)
			{
				if(EjectLib(ProcessHandle, pid, DLLName))
				{
					SetDlgItemText(hDlg, ID_RESPONSE, "Dll successfully ejected!");
				}
				else
				{
					SetDlgItemText(hDlg, ID_RESPONSE, "There was an error ejecting the dll");
				}
			}
			SetFocus(GetDlgItem(hDlg, IDPROCESS));
		}

		return TRUE;

		break;
	}
	return FALSE;
}
