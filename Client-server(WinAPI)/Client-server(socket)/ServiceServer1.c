
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")
#include <WinSock2.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h>
#include <winsvc.h>
#define SOCKET_PORT 7070
#define MAXCOUNT 5
#define BACKLOG 20
#define EVENTLOGNAME _T("holder")
#define PRINTF(str) reportEvent(_T(str))
#define PRINTFd(str, number)  { \
    TCHAR buf[100] = _T(""); \
    _tprintf(buf, _T(str), number); \
    reportEvent(buf); \
}


void reportEvent(const TCHAR* const message) {
    HANDLE handleEventLog = RegisterEventSource(NULL, EVENTLOGNAME);
    if (handleEventLog != NULL) {
        LPCTSTR strings[1];
        strings[0] = message;
        ReportEvent(handleEventLog,
                    EVENTLOG_ERROR_TYPE,
                    0,
                    (DWORD)0xC0020001L,
                    NULL,
                    1,
                    0,
                    strings,
                    NULL);
        DeregisterEventSource(handleEventLog);
    }
}

enum command {
    WriteDouble,
    Clean,
    GetAVG,
    GetSampleVariance,
    Exit
};

struct params {
    int real_socket_file;
    int* lastNumber;
    int* firstNumber;
    double* numbers;
    int* count;
    CRITICAL_SECTION* criticalSection;
};

DWORD WINAPI worker(LPVOID param) {
    int res = 0;
    struct params* params = (struct params*)param;
    do {
        enum command cmd;
        res = recv(params->real_socket_file, (char*)&cmd, sizeof(cmd), 0);
        if (res == SOCKET_ERROR) {
          PRINTFd("recv #1 error: %d\n",
                        WSAGetLastError())
        } else if (res != 0) {
            if (res != sizeof(cmd)) {
                PRINTFd("unexcpected size recv #1 %d\n", WSAGetLastError())
                    res = -1;
            } else {
                int res2 = 0, i = 0, current = 0;
                double number = 0;
                switch (cmd) {
                case WriteDouble: {
                    int res2 = recv(params->real_socket_file, (char*)&number,
                                    sizeof(double), 0);
                    if (res2 == SOCKET_ERROR) {
                        PRINTFd("recv #2 error: %d\n", WSAGetLastError())
                            res = -1;
                    } else if (res2 == 0) {
                        PRINTF("unexpected close socket\n");
                        res = -1;
                    } else if (res2 != sizeof(double)) {
                        PRINTF("unexcpected size recv #2");
                        res = -1;
                    }
                    if (res > 0) {
                        if (number < 0) {
                            PRINTF("memory overflow attack\n");
                            res = -1;
                        } else {
                            EnterCriticalSection(params->criticalSection);
                            *params->lastNumber =
                                    (*params->lastNumber + 1) % MAXCOUNT;
                            if (*(params->count) == MAXCOUNT) {
                                *params->firstNumber =
                                        (*params->firstNumber + 1) % MAXCOUNT;
                            } else {
                                (*(params->count))++;
                            }
                            params->numbers[*params->lastNumber] = number;
                            LeaveCriticalSection(params->criticalSection);
                        }
                    }
                    break;
                }
                case GetAVG: {
                    EnterCriticalSection(params->criticalSection);
                    double sum = 0;
                    for (int i = 0; i < *(params->count); i++,
                                    current = (current + 1) % MAXCOUNT)
                            sum += params->numbers[current];
                    double avg = sum / *(params->count);
                    LeaveCriticalSection(params->criticalSection);
                    res2 = send(params->real_socket_file, (char*)&avg,
                                    sizeof(double), 0);
                    if (res2 == SOCKET_ERROR) {
                        PRINTFd("send #1 error: %d\n", WSAGetLastError())
                            res = -1;
                    } else if (res2 != sizeof(double)) {
                        PRINTF("unexpected size send\n");
                        res = -1;
                    }
                    break;
                }
                case Clean: {
                    EnterCriticalSection(params->criticalSection);
                    for (int i = 0; i < *(params->count); i++,
                                    current = (current + 1) % MAXCOUNT)
                            params->numbers[current] = 0;
                    *params->count = 0;
                    *params->lastNumber = -1;
                    *params->firstNumber = 0;
                    LeaveCriticalSection(params->criticalSection);
                    break;
                }
                case GetSampleVariance: {
                    double sum = 0;
                    EnterCriticalSection(params->criticalSection);
                    double n = (*(params->count));
                    for (int i = 0; i < *(params->count); i++)
                            sum += params->numbers[i];
                    const double m = (1. / n) * sum;
                    sum = 0;
                    for (int i = 0; i < *(params->count); i++) {
                        double current_operand = params->numbers[i] - m;
                        current_operand *= current_operand;
                        sum += current_operand;
                    }
                    double disp = (1. / (n - 1)) * sum;
                    LeaveCriticalSection(params->criticalSection);
                    res2 = send(params->real_socket_file, (char*)&disp,
                                    sizeof(double), 0);
                    if (res2 == SOCKET_ERROR) {
                        PRINTFd("send #1 error: %d\n", WSAGetLastError())
                            res = -1;
                    } else if (res2 != sizeof(double)) {
                        PRINTF("unexpected size send\n");
                        res = -1;
                    }
                    break;
                }
                case Exit: {
                    res = 0;
                    break;
                }
                default: break;
                }
            }
        }
    } while (res > 0);
    if (closesocket(params->real_socket_file) == SOCKET_ERROR)
            PRINTFd("closesocket2 :%d\n", GetLastError())
        free(params);
    return 0;
}

DWORD WINAPI threadMainLoop(LPVOID p) {
    int res = 0;
    struct params* prm = (struct params*)p;
    do {
        int real_socket_file = accept(prm->real_socket_file, NULL, NULL);
        if (real_socket_file == INVALID_SOCKET) { PRINTFd("accept error: %d\n",
                        WSAGetLastError())
        } else {
            struct params* params =
                    (struct params*)malloc(sizeof(struct params));
            if (params == NULL) {
                res = -1;
                PRINTF("malloc error\n");
            } else {
                ZeroMemory(params, sizeof(params));
                params->real_socket_file = real_socket_file;
                params->count = prm->count;
                params->criticalSection = prm->criticalSection;
                params->firstNumber = prm->firstNumber;
                params->lastNumber = prm->lastNumber;
                params->numbers = prm->numbers;
                HANDLE thread = CreateThread(NULL, 0, worker, params, 0, NULL);
                if (thread == NULL)
                  PRINTF("error create thread\n");
                else
                  CloseHandle(thread);
            }
        }
    } while (true);
    res = closesocket(prm->real_socket_file);
    if (res == SOCKET_ERROR) PRINTF("closesocket\n");
}

bool exiting = 0;
HANDLE mainThread;

void WINAPI handler(DWORD dwControl) {
    if (dwControl == SERVICE_CONTROL_STOP) {
        exiting = 1;
        if (!TerminateThread(mainThread, 0)) {
            PRINTFd("terminated thread :%d\n", GetLastError())
        }
    }
}

void WINAPI serviceProc(DWORD dwNumServicesArgs,
                LPTSTR* lpServiceArgVectors) {
    SERVICE_STATUS_HANDLE statushandler =
            RegisterServiceCtrlHandler(_T("holder"), handler);
    if (statushandler == 0) {
        PRINTFd("register service ctrl handler :%d\n", GetLastError())
    } else {
        SERVICE_STATUS serviceStatus;
        serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        serviceStatus.dwCurrentState = SERVICE_START_PENDING;
        serviceStatus.dwControlsAccepted = 0;
        serviceStatus.dwWin32ExitCode = 0;
        serviceStatus.dwServiceSpecificExitCode = 0;
        serviceStatus.dwCheckPoint = 0;
        serviceStatus.dwWaitHint = 0;
        if (!SetServiceStatus(statushandler, &serviceStatus)) {
            PRINTFd("set service status :%d\n", GetLastError())
        } else {
            serviceStatus.dwCurrentState = SERVICE_RUNNING;
            serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
            if (!SetServiceStatus(statushandler, &serviceStatus)) {
                PRINTFd("set service status :%d\n", GetLastError())
            } else {
                int res = 0;
                WSADATA wsadata = { 0 };
                res = WSAStartup(MAKEWORD(2, 2), &wsadata);
                if (res != 0) {
                    PRINTFd("error of init lib: %d\n", GetLastError())
                } else {
                    int socket_file = socket(AF_INET, SOCK_STREAM, 0);
                    if (socket_file == INVALID_SOCKET) {
                        PRINTFd("socket error: %d\n", WSAGetLastError())
                    } else {
                        struct sockaddr_in address;
                        address.sin_family = AF_INET;
                        address.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
                        address.sin_port = SOCKET_PORT;
                        res = bind(socket_file, (const sockaddr*)&address,
                                        sizeof(address));
                        if (res == SOCKET_ERROR) {
                          PRINTFd("bind error: %d\n", WSAGetLastError())
                        } else {
                            int lastNumber = -1, firstNumber = 0;
                            double numbers[MAXCOUNT];
                            for (int i = 0; i < MAXCOUNT; i++) numbers[i] = 0;
                            int count = 0;
                            res = listen(socket_file, BACKLOG);
                            if (res == SOCKET_ERROR) {
                              PRINTFd("listen error: %d\n", WSAGetLastError())
                            } else {
                                CRITICAL_SECTION criticalSection;
                                InitializeCriticalSection(&criticalSection);
                                struct params* params =
                                        (struct params*)malloc(
                                                        sizeof(struct params));
                                if (params == NULL) {
                                    res = -1;
                                    PRINTF("malloc error\n");
                                } else {
                                    params->real_socket_file = socket_file;
                                    params->count = &count;
                                    params->criticalSection = &criticalSection;
                                    params->firstNumber = &firstNumber;
                                    params->lastNumber = &lastNumber;
                                    params->numbers = &numbers[0];
                                    mainThread = CreateThread(NULL, 0,
                                                              threadMainLoop,
                                                              params, 0, NULL);
                                    if (mainThread == NULL) {
                                      PRINTFd("create thread: %d\n",
                                        GetLastError())
                                    } else {
                                        while (!exiting) {}
                                        serviceStatus.dwCurrentState =
                                                SERVICE_STOP_PENDING;
                                        serviceStatus.dwControlsAccepted = 0;
                                        if (!SetServiceStatus(
                                                             statushandler,
                                                             &serviceStatus)) {
                                            PRINTFd("set service status :%d\n",
                  GetLastError())
                                        }
                                    }
                                    DeleteCriticalSection(&criticalSection);
                                    free(params);
                                }
                            }
                        }
                        res = closesocket(socket_file);
                        if (res == SOCKET_ERROR) PRINTF("closesocket\n");
                    }
                }
                serviceStatus.dwCurrentState = SERVICE_STOPPED;
                serviceStatus.dwControlsAccepted = 0;
                if (!SetServiceStatus(statushandler, &serviceStatus)) {
                    PRINTFd("set service status :%d\n", GetLastError())
                }
                WSACleanup();
            }
        }
    }
}

int main() {
    int res = 0;
    SERVICE_TABLE_ENTRY serviceTableEntry;
    TCHAR name[] = _T("holder");
    serviceTableEntry.lpServiceName = name;
    serviceTableEntry.lpServiceProc = serviceProc;
    if (!StartServiceCtrlDispatcher(&serviceTableEntry)) {
        res = -1;
        PRINTFd("StartSErviceCtrlDispatcher :%d\n", GetLastError())
    }
    return res;
}
