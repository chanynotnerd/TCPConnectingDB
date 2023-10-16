
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
using namespace std;

#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32")

int main()
{
	// Winsock �ʱ�ȭ, ���� ����ϱ� ���� �� �ʱ�ȭ �ؾ���, �׳� ù ����
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// ���� ����, 
	SOCKET ServerSocket = socket(PF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN ServerSock;

	// �� ��Ʈ�� �ش��ϴ� �� �־��ֱ�
	ZeroMemory(&ServerSock, 0);
	ServerSock.sin_family = PF_INET;
	ServerSock.sin_port = htons(7777);
	inet_pton(AF_INET,
		"127.0.0.1", &(ServerSock.sin_addr.s_addr));

	// connect()�� ���� Ŭ���̾�Ʈ�� ������ ���� ��û.
	connect(ServerSocket, (SOCKADDR*)&ServerSock, sizeof(ServerSock));


	while (1)
	{
		char Message[1024] = { 0, };
		cin.getline(Message, sizeof(Message));
		send(ServerSocket, Message, strlen(Message), 0);

		// quit �ϸ� Ŭ���̾�Ʈ ����
		if (strcmp(Message, "quit") == 0) {
			break;
		}
	}

	closesocket(ServerSocket);
	WSACleanup();

	return 0;
}