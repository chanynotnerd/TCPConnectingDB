
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
using namespace std;

#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32")

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

	closesocket(ServerSocket);
	WSACleanup();

	return 0;
}