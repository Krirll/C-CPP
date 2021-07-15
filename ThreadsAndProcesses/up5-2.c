/*Перед каждым именем файла поставить его номер в таком порядке, который выдает readdir*/
#define _CRT_SECURE_NO_WARNINGS
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h> //библиотека для работы с WinAPI
#include <tchar.h> //для написания универсального кода, который можно скомпилировать для однобайтовых, 
//многобайтовых кодировок или наборов символов Юникода
#include <stdbool.h>
#include <stdio.h>
#include <strsafe.h> //Функции, определенные в Strsafe.h, предусматривают 
//дополнительную обработку для правильной работы буфера в  коде
void fromIntToChar(int convert, TCHAR* str);
void ErrorExit(LPCTSTR lpszFunction);
int renaming(WIN32_FIND_DATA findDATA, int count_file, int error, TCHAR Name[], TCHAR NewName[], TCHAR buffer[]);
int newDir(TCHAR buffer[]);

typedef struct listPid {
	HANDLE id;
	struct listPid* next;
}list;

void fromIntToChar(int convert, TCHAR* str) { //функция для перевода счетчика типа int в тип char
	for (int i = convert; i >= 10; i /= 10) { //проход до конца строки для записи числа //100 -> 10 -> 1
		str++;
	}
	for (; convert > 0;) {
		*str = convert % 10 + '0';
		str--; //идем от конца строки к началу, сначала записываются единицы, потом десятки, потом сотни и т.д.
		convert /= 10;
	}
}

int renaming(WIN32_FIND_DATA findDATA, int count_file, int error, TCHAR Name[], TCHAR NewName[], TCHAR buffer[]) {
	TCHAR CountFile[1024] = _T("");
	TCHAR* charCount = CountFile;
	fromIntToChar(count_file, CountFile);
	_tcscat(NewName, CountFile); //это версии strcat для расширенных символов и многобайтовых символов
	_tcscat(NewName, findDATA.cFileName);
	_tcscat(Name, findDATA.cFileName);
	if (!MoveFile(Name, NewName)) { //переименовывание файла и проверка на успешность выполнения 
		ErrorExit(_T("error:> MoveFile"));
		error = -1;
	}
	_tcscpy(Name, buffer);
	_tcscpy(NewName, buffer);
	return ++count_file;
}


void ErrorExit(LPCTSTR lpszFunction) { //window of error with info about it
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

DWORD WINAPI ThreadFunction(LPVOID param);

int newDir(TCHAR buffer[]) {
	WIN32_FIND_DATA findDATA; //struct for working with files in windows
	DWORD Error = 0, error = 0; //DWORD - 32-битное беззнаковое целое. Аналоги: unsigned long int, UINT.
	HANDLE hFind = FindFirstFile(buffer, &findDATA);
	//HANDLE - дескриптор объекта; в данном случае получение дескриптора первого файла в директории
	bool end_flag = false;
	int count_file = 1;
	list* first = 0, *elem;
	 //TCHAR - Type + char: Для многобайтового набора символов: TCHAR означает char (простой символ 1 байт)
	//для набора символов Unicode : TCHAR означает wchar(широкий символ 2 байта)
	if (INVALID_HANDLE_VALUE == hFind) { //В случае успешного создания или открытия файла, процедура CreateFile возвращает его хэндл.
		//В случае ошибки возвращается специальное значение INVALID_HANDLE_VALUE
		ErrorExit(_T("error:> FindFirstFile"));//вывод ошибки функции на экран с описанием
		error = -1;
		end_flag = true;
	}
	if (!end_flag) {
		do {
			if ((findDATA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				TCHAR Name[255] = _T(""), NewName[255] = _T("");
				_tcsncpy(Name, buffer, _tcslen(buffer) - 1);
				_tcsncpy(NewName, buffer, _tcslen(buffer) - 1);
				count_file = renaming(findDATA, count_file, error, Name, NewName, buffer);
			}
			else if ((findDATA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
				((_tcscmp(findDATA.cFileName, TEXT(".")) != 0) && (_tcscmp(findDATA.cFileName, TEXT("..")) != 0))) {
				TCHAR name[1024] = _T(""), copyBuf[1024] = _T("");
				_tcsncpy(name, buffer, _tcslen(buffer) - 1);
				name[_tcslen(buffer) - 1] = _T('\0');
				_tcscat(name, findDATA.cFileName);
				_tcscat(name, _T("\\*"));
				size_t sizeArg = _tcslen(name);
				TCHAR* arg = (TCHAR*)malloc((sizeArg + 1) * sizeof(arg));
				if (arg == NULL) ErrorExit(_T("malloc"));
				else {
					_tcscpy(arg, name);
					HANDLE handle = CreateThread(NULL, 0, ThreadFunction, arg, 0, NULL);
					if (handle == NULL) {
						ErrorExit(_T("CreateThread"));
					}
					else {
						elem = (struct listPid*)malloc(sizeof(struct listPid));
						if (elem) {
							elem->id = handle;
							elem->next = first;
							first = elem;
						}
					}
				}
			}
		} while (FindNextFile(hFind, &findDATA) != NULL);
	} Error = GetLastError();
	if (Error != ERROR_NO_MORE_FILES) {
		ErrorExit(_T("error:> FindNextFile "));
		error = -1;
	}
	error = FindClose(hFind);
	if (error == 0) {
		ErrorExit(_T("error:> FindClose "));
		error = -1;
	}
	//wait here
	while (first != 0) {
		DWORD result_of_wait = WaitForSingleObject(first->id, INFINITE);
		if (first->id != 0) {
			CloseHandle(first->id);
			elem = first;
			first = first->next;
			free(elem);
		}
		if (result_of_wait == WAIT_FAILED) ErrorExit(_T("Error of waiting thread"));
	}
	free(first);
	return error;
}

int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	TCHAR buffer[1024] = _T("C:\\Users\\1\\Desktop\\praktan6\\*");
	int res = newDir(buffer);
	_CrtDumpMemoryLeaks();
	return res;
}

DWORD WINAPI ThreadFunction(LPVOID param) {
	TCHAR* buf = (TCHAR*)param;
	_tprintf(_T("%s\n"), buf);
	newDir(buf);
	free(buf);
	return 0;
}