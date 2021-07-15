#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <tchar.h>
#include <stdbool.h>
#include <stdio.h>
#include <strsafe.h>
void _itoa(int convert, TCHAR* str);

void _itoa(int convert, TCHAR* str) {
	for (int i = convert; i >= 10; i /= 10) {
		str++;
	}
	for (; convert > 0;) {
		*str = convert % 10 + '0';
		str--;
		convert /= 10;
	}
}

void ErrorExit(LPCTSTR lpszFunction) {
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);
	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

int main() {
	WIN32_FIND_DATA findDATA;
	DWORD Error = 0;
	DWORD error = 0;
	HANDLE hFind = FindFirstFile(_T("C:\\Users\\User\\Desktop\\praktan6\\*"), &findDATA);
	bool end_flag = false;
	int count_file = 0;
	TCHAR CountFile[255] = TEXT("");
	TCHAR* counter = CountFile;
	TCHAR Name[255] = _T("C:\\Users\\User\\Desktop\\praktan6\\");
	TCHAR NewName[255] = _T("C:\\Users\\User\\Desktop\\praktan6\\");
	if (INVALID_HANDLE_VALUE == hFind) {
		ErrorExit(_T("error:> FindFirstFile"));
		error = -1;
		end_flag = true;
	}
	if (!end_flag) {
		do {
			if ((findDATA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				count_file++;
				_itoa(count_file, counter);
				_tcscat(NewName, CountFile);
				_tcscat(NewName, findDATA.cFileName);
				_tcscat(Name, findDATA.cFileName);
				if (!MoveFile(Name, NewName)) {
					ErrorExit(_T("error:> MoveFile"));
					error = -1;
				}
				_tcscpy(Name, TEXT("C:\\Users\\User\\Desktop\\praktan6\\")); //папка находится в папке колледж, 2 курс 2 семестр
				_tcscpy(NewName, TEXT("C:\\Users\\User\\Desktop\\praktan6\\"));
			}
		} while (FindNextFile(hFind, &findDATA) != NULL);
	} Error = GetLastError();
	if (Error != ERROR_NO_MORE_FILES) {
		ErrorExit(_T("error:> FindNextFile"));
		error = -1;
	}
	FindClose(hFind);
	return error;
}

