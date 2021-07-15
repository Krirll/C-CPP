//client
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")
#include <WinSock2.h>
#include <stdio.h>
#define SOCKET_PORT 7070
#define MAXNOTELENGTH 100
#define MAXCOUNT 5
enum command {
	WriteDouble,
	Clean,
	GetAVG,
	GetSampleVariance,
	Exit
};

int main() {
	int res = 0;
	FILE* logClient = fopen("logClient.txt", "w");
	if (logClient == NULL) {
		perror("error of opening logClient\n");
	} 
	else {
		WSADATA wsadata = { 0 };
		res = WSAStartup(MAKEWORD(2, 2), &wsadata);
		if (res != 0) {
			fprintf(logClient, "Error of initializing library\n");
		}
		else {
			int socket_file = socket(AF_INET, SOCK_STREAM, 0);
			if (socket_file == INVALID_SOCKET) {
				fprintf(logClient, "socket error: %d\n", WSAGetLastError());
			}
			else {
				struct sockaddr_in address;
				address.sin_family = AF_INET;
				address.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
				address.sin_port = SOCKET_PORT;
				res = connect(socket_file, (struct sockaddr*)&address, sizeof(address)); //устанавливает соединение с указанным сокетом
				if (res == SOCKET_ERROR) {
					fprintf(logClient, "connect error: %d\n", WSAGetLastError());
				}
				else {
					bool flag = 1;
					do {
						fseek(stdin, 0, SEEK_END); //перемещает указатель позиции в потоке
						printf("\n1 - write number, 2 - get avg, 3 - clean, 4 - get sample variance, 0 - exit: ");
						int c = getchar();
						switch (c) {
							case '1': {
								double number;
								printf("enter a number: ");
								res = scanf("%lf", &number);
								if (res == EOF) {
									fprintf(logClient, "scanf\n");
									res = -1;
								}
								else {
									enum command cmd = WriteDouble;
									res = send(socket_file, (char*)&cmd, sizeof(cmd), 0);
									if (res == SOCKET_ERROR) {
										fprintf(logClient, "send1 error: %d\n", WSAGetLastError());
									}
									else if (res < sizeof(cmd)) {
										fprintf(logClient, "unexpected size in send\n");
									}
									else {
										res = send(socket_file, (char*)&number, sizeof(double), 0);
										if (res == SOCKET_ERROR) {
											fprintf(logClient, "send2 error: %d\n", WSAGetLastError());
										}
										else if (res < sizeof(double)) {
											fprintf(logClient, "unexpected size in send\n");
										}
									}
								}
								break;
							}
							case '2': {
								enum command cmd = GetAVG;
								res = send(socket_file, (char*)&cmd, sizeof(cmd), 0);
								if (res == SOCKET_ERROR) {
									fprintf(logClient, "send3 error: %d\n", WSAGetLastError());
								}
								else if (res < sizeof(cmd)) {
									fprintf(logClient, "unexpected size in send\n");
								}
								else {
									double number = 0;
									int res2 = recv(socket_file, (char*)&number, sizeof(number), 0);
									if (res2 == SOCKET_ERROR) {
										fprintf(logClient, "recv #2 error: %d\n", WSAGetLastError());
										res = -1;
									}
									else if (res2 == 0) {
										fprintf(logClient, "unexpected close socket");
										res = -1;
									}
									else if (res2 != sizeof(double)) {
										fprintf(logClient, "unexcpected size recv #2");
										res = -1;
									}
									else {
										/*if (number > 0)*/ printf("avg = %lf\n", number);
									}
								}
								break;
							}
							case '3': {
								enum command cmd = Clean;
								res = send(socket_file, (char*)&cmd, sizeof(cmd), 0);
								if (res == SOCKET_ERROR) {
									fprintf(logClient, "send4 error: %d\n", WSAGetLastError());
								}
								else if (res < sizeof(cmd)) {
									fprintf(logClient, "unexpected size in send\n");
								}
								break;
							}
							case '4': {
								enum command cmd = GetSampleVariance;
								res = send(socket_file, (char*)&cmd, sizeof(cmd), 0);
								if (res == SOCKET_ERROR) {
									fprintf(logClient, "send5 error: %d\n", WSAGetLastError());
								}
								else if (res < sizeof(cmd)) {
									fprintf(logClient, "unexpected size in send");
								}
								else {
									double disp = 0;
									int res2 = recv(socket_file, (char*)&disp, sizeof(disp), 0);
									if (res2 == SOCKET_ERROR) {
										fprintf(logClient, "recv #2 error: %d\n", WSAGetLastError());
										res = -1;
									}
									else if (res2 == 0) {
										fprintf(logClient, "unexpected close socket");
										res = -1;
									}
									else if (res2 != sizeof(double)) {
										fprintf(logClient, "unexcpected size recv #2");
										res = -1;
									}
									else {
										/*if (disp != 0)*/ printf("sample variance = %lf\n", disp);
									}
								}
								break;
							}
							case '0': {
								flag = 0;
								break;
							}
							default: printf("\nincorrect command!\n");
						}
					} while (flag && res != -1);
				} 
				res = closesocket(socket_file);
				if (res == SOCKET_ERROR) {
					fprintf(logClient, "closesocket error: %d\n", WSAGetLastError());
				}
			}	
			WSACleanup();
		}
	}
	res = fclose(logClient);
	if (res == EOF) {
		res = -1;
		perror("error fclose\n");
	}
	return res;
}
