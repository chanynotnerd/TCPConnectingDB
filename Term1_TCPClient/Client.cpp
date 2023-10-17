
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
using namespace std;

#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32")

void ReceiveMessages(SOCKET ServerSocket) {
	char buffer[1024];
	while (1) {
		int recvlen = recv(ServerSocket, buffer, sizeof(buffer), 0);
		if (recvlen > 0) {
			buffer[recvlen] = '\0';
			cout << "Received from server: " << buffer << endl;
		}
	}
}


int main()
{
	// Winsock 초기화, 소켓 사용하기 전에 꼭 초기화 해야함, 그냥 첫 순서
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// 소켓 생성, 
	SOCKET ServerSocket = socket(PF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN ServerSock;

	// 각 포트에 해당하는 값 넣어주기
	ZeroMemory(&ServerSock, 0);
	ServerSock.sin_family = PF_INET;
	ServerSock.sin_port = htons(7777);
	inet_pton(AF_INET,
		"127.0.0.1", &(ServerSock.sin_addr.s_addr));

	// connect()를 통해 클라이언트가 서버에 연결 요청.
	connect(ServerSocket, (SOCKADDR*)&ServerSock, sizeof(ServerSock));

	// 클라이언트의 이름 입력
	cout << "Enter your name: ";
	char ClientName[100];
	cin.getline(ClientName, sizeof(ClientName));

	// 이름을 서버로 전송
	send(ServerSocket, ClientName, strlen(ClientName), 0);

	// 42~54줄은 이름 중복검사 용도
	char response[100];
	int recvlen = recv(ServerSocket, response, sizeof(response), 0);
	response[recvlen] = '\0';

	cout << "Received from server: " << response << endl;

	if (strcmp(response, "DuplicatedName") == 0) {
		cout << "Name already exists. Please choose a different name: ";
		cin.getline(ClientName, sizeof(ClientName));

		// 다시 이름을 서버로 전송
		send(ServerSocket, ClientName, strlen(ClientName), 0);
	}
	else {
		cout << "Connected successfully. You can start sending messages." << endl;
	}


	/*
	// 중복 이름 메시지를 감지한 경우 새 이름을 요청
	if (strcmp(response, "DuplicateName") == 0) {
		// cout << "Name already exists. Please choose a different name: ";
		cin.getline(ClientName, sizeof(ClientName));

		// 다시 이름을 서버로 전송
		send(ServerSocket, ClientName, strlen(ClientName), 0);
	}
	else
	{
		cout << "Connected sucessfully. You can start sending Messages." << endl;
	}
	*/
	// 스레드 생성 및 ReceiveMessages 함수 실행
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReceiveMessages, (LPVOID)ServerSocket, 0, NULL);


	while (1)
	{
		char Message[1024] = { 0, };
		cin.getline(Message, sizeof(Message));
		send(ServerSocket, Message, strlen(Message), 0);

		// quit 하면 클라이언트 종료
		if (strcmp(Message, "quit") == 0) {
			break;
		}
	}

	CloseHandle(hThread);
	closesocket(ServerSocket);
	WSACleanup();

	return 0;
}