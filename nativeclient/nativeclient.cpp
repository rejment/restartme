#define UNICODE
#include <windows.h>

#define CLASSNAME L"rejment/restartme"
#define PROCNAME L"tcstudio"
#define MESSAGE_ID 666

#pragma intrinsic(strlen)
#pragma comment(linker, "/MERGE:.rdata=.text")

wchar_t filename[MAX_PATH*10];
wchar_t argument[MAX_PATH*10];

BOOL sendmsg(HWND wnd, LPWSTR message)
{
	COPYDATASTRUCT cds;
    cds.cbData = (DWORD) (lstrlen(message)+1) * sizeof(wchar_t);
	cds.dwData = MESSAGE_ID;
	cds.lpData = (void*) message;

	DWORD dwResult;
	LRESULT result = SendMessageTimeout((HWND)wnd, WM_COPYDATA, 0, (LPARAM) &cds, SMTO_ABORTIFHUNG, 5000, &dwResult); 
	if (result == 0 && GetLastError() == 0)
		dwResult = -1;

    return result != -1;
}

wchar_t * __cdecl strchr (wchar_t * string, int ch)
{
        while (*string && *string != (wchar_t)ch) string++;
        if (*string == (wchar_t)ch) return((wchar_t *)string);
        return((wchar_t*)string);
}

wchar_t * __cdecl strrchr (wchar_t * string, int ch)
{
        wchar_t *start = (wchar_t *)string;
        while (*string++);
        while (--string != start && *string != (wchar_t)ch);
        if (*string == (wchar_t)ch) return( (wchar_t *)string );
        return(string);
}

void WinMainCRTStartup()
{

	LPWSTR message = GetCommandLine();
    //MessageBox(0, message, L"native_debug", MB_OK);

	wchar_t *args = strchr(message, ' ') + 1;
	if (message[0] == '"')
		args = strchr(message+2, '"') + 1;

	HWND found = FindWindowEx(HWND_MESSAGE, NULL, CLASSNAME, PROCNAME);
	while (found != 0) {
		if (sendmsg(found, args)) {
			ExitProcess(0);
		}
		found = FindWindowEx(HWND_MESSAGE, found, CLASSNAME, PROCNAME);
	}

	GetModuleFileName(0, filename, MAX_PATH-1);
	strrchr(filename, '\\')[1] = 0;
	while (*args!=0 && *args == ' ') args++;
	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi;
	si.cb = sizeof(si);
	SetCurrentDirectory(filename);
    lstrcpy(argument, L"studio.exe ");
    lstrcat(argument, args);
	CreateProcess(0, argument, 0, 0, 0, 0, 0, filename, &si, &pi);
	ExitProcess(0);
}
