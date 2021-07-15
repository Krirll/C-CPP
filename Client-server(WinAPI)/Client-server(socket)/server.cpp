/*7-0-2 Создать сервер и клиент на Windows, работающие по протоколу TCP, использующие бинарный протокол.
Клиент должен позволять передавать серверу вещественные числа и получать :
-среднее арифметическое,
выборочную дисперсию введенных чисел.
Клиент после подключения может передавать четыре команды :
-зарегистрировать число,
очистить набор чисел,
получить среднее арифметическое,
получить дисперсию.
Реализовать сервер в виде сервиса Windows.Все возможные ошибки должны проверяться и протоколироваться.*/

//#define _WINSOCK_DEPRECATED_NO_WARNINGS
//#define _CRT_SECURE_NO_WARNINGS
//#pragma comment(lib, "Ws2_32.lib")
//#include <WinSock2.h>
//#include <stdio.h>
//#define SOCKET_PORT 7070
//#define MAXCOUNT 5
//#define BACKLOG 20
//
//enum command {
//	WriteDouble,
//	Clean,
//	GetAVG,
//	GetSampleVariance,
//	Exit
//};
//
//int main() {
//	int res = 0;
//	FILE* logServer = fopen("logServer.txt", "w");
//	if (logServer == NULL) {
//		perror("error of opening file\n");
//		res = -1;
//	} 
//	else {
//		WSADATA wsadata = { 0 }; //инициализация библиотеки
//		res = WSAStartup(MAKEWORD(2, 2), &wsadata);  //в скобках версия библиотеки
//		if (res != 0) { //проверка инициализации
//			fprintf(logServer, "error of init lib\n");
//		}
//		else {
//			int socket_file = socket(AF_INET, SOCK_STREAM, 0); //создание сокета
//			if (socket_file == INVALID_SOCKET) {
//				fprintf(logServer, "socket error: %d\n", WSAGetLastError());
//			}
//			else {
//				struct sockaddr_in address; //инициализация сокета
//				address.sin_family = AF_INET;
//				address.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); //переводит из одного формата в другой(число)
//				address.sin_port = SOCKET_PORT;
//				res = bind(socket_file, (const sockaddr*)&address, sizeof(address)); //связывает сокет с адресом и номером порта, указанными в addr
//				if (res == SOCKET_ERROR) fprintf(logServer, "bind error: %d\n", WSAGetLastError());//
//				else {
//					int lastNumber = -1, firstNumber = 0;
//					double numbers[MAXCOUNT];
//					numbers[0] = 0;
//					numbers[1] = 0;
//					numbers[2] = 0;
//					numbers[3] = 0;
//					numbers[4] = 0;
//					int count = 0;
//					res = listen(socket_file, BACKLOG); //ожидает ("слушает") запросы связи на данном сокете
//					if (res == SOCKET_ERROR) fprintf(logServer, "listen error: %d\n", WSAGetLastError());
//					else {
//						do {
//							int real_socket_file = accept(socket_file, NULL, NULL); //для принятия связи на сокет
//							//возвращает новый сокет-дескриптор, через который и происходит общение клиента с сервером
//							if (real_socket_file == INVALID_SOCKET) fprintf(logServer, "accept error: %d\n", WSAGetLastError());
//							else {
//								do {
//									enum command cmd;
//									res = recv(real_socket_file, (char*)&cmd, sizeof(cmd), 0); //для чтения данных из сокета
//									if (res == SOCKET_ERROR) fprintf(logServer, "recv #1 error: %d\n", WSAGetLastError());
//									else if (res != 0) {
//										if (res != sizeof(cmd)) {
//											fprintf(logServer, "unexcpected size recv #1");
//											res = -1;
//										}
//										else {
//											int res2 = 0, i = 0, current = 0;
//											double number = 0;
//											switch (cmd) {
//												case WriteDouble: {
//												//добавление числа
//													int res2 = recv(real_socket_file, (char*)&number, sizeof(double), 0);
//													if (res2 == SOCKET_ERROR) {
//														fprintf(logServer, "recv #2 error: %d\n", WSAGetLastError());
//														res = -1;
//													}
//													else if (res2 == 0) {
//														fprintf(logServer, "unexpected close socket");
//														res = -1;
//													}
//													else if (res2 != sizeof(double)) {
//														fprintf(logServer, "unexcpected size recv #2");
//														res = -1;
//													}
//													fprintf(logServer, "got number -> %lf\n", number);
//													if (res > 0) {
//														if (number < 0) {
//															fprintf(logServer, "memory overflow attack");
//															res = -1;
//														}
//														else {
//															lastNumber = (lastNumber + 1) % MAXCOUNT;
//															if (count == MAXCOUNT) {
//																firstNumber = (firstNumber + 1) % MAXCOUNT;
//															}
//															else {
//																count++;
//															}
//															numbers[lastNumber] = number;
//														}
//													}
//													break;
//												}
//												case GetAVG: {
//													//получение среднего арифмитического набора чисел
//													double sum = 0;
//													for (int i = 0; i < count; i++, current = (current + 1) % MAXCOUNT) sum += numbers[current];
//													double avg = sum / count;
//													fprintf(logServer, "%lf / %d = %lf\n", sum, count, avg);
//													res2 = send(real_socket_file, (char*)&avg, sizeof(double), 0);
//													if (res2 == SOCKET_ERROR) {
//														fprintf(logServer, "send #1 error: %d\n", WSAGetLastError());
//														res = -1;
//													}
//													else if (res2 != sizeof(double)) {
//														fprintf(logServer, "unexpected size send\n");
//														res = -1;
//													}
//													break;
//												}
//												case Clean: {
//													//очистка набора чисел
//													for (int i = 0; i < count; i++, current = (current + 1) % MAXCOUNT) numbers[current] = 0;
//													count = 0;
//													lastNumber = -1;
//													firstNumber = 0;
//													break;
//												}
//												case GetSampleVariance: {
//													//нахождение выборочной дисперсии
//													double sum = 0;
//													double n = count;
//													for (int i = 0; i < count; i++) sum += numbers[i];
//													const double m = (1. / n) * sum;
//													sum = 0;
//													for (int i = 0; i < count; i++) {
//														double current_operand = numbers[i] - m;
//														current_operand *= current_operand;
//														sum += current_operand;
//													}
//													double disp = (1. / (n - 1)) * sum;
//													fprintf(logServer, "dips = %lf\n", disp);
//													res2 = send(real_socket_file, (char*)&disp, sizeof(double), 0);
//													if (res2 == SOCKET_ERROR) {
//														fprintf(logServer, "send #1 error: %d\n", WSAGetLastError());
//														res = -1;
//													}
//													else if (res2 != sizeof(double)) {
//														fprintf(logServer, "unexpected size send\n");
//														res = -1;
//													}
//														break;
//												}
//												case Exit: {
//													res = 0;
//													break;
//												}
//											} //close switch
//										} //else recv
//									} //else if res != 0
//								} while (res > 0); //res == 0, сокет закрыт клиентом, если меньше то ошибка
//								if (closesocket(real_socket_file) == SOCKET_ERROR) fprintf(logServer, "closesocket2\n");
//							} //else accept
//						} while (res > 0);
//					} //else listen
//				} //else bind
//				res = closesocket(socket_file); //проверка закрытия сокета
//				if (res == SOCKET_ERROR) fprintf(logServer, "closesocket");
//			}
//			WSACleanup(); //освобождение ресурсов библиотеки
//		}
//	}
//	res = fclose(logServer);
//	if (res == EOF) {
//		res = -1;
//		perror("error fclose\n");
//	}
//	return res;
//}
