
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

	// Ŭ���̾�Ʈ�� �̸� �Է�
	cout << "Enter your name: ";
	char ClientName[100];
	cin.getline(ClientName, sizeof(ClientName));

	// �̸��� ������ ����
	send(ServerSocket, ClientName, strlen(ClientName), 0);

	// 42~54���� �̸� �ߺ��˻� �뵵
	char response[100];
	int recvlen = recv(ServerSocket, response, sizeof(response), 0);
	response[recvlen] = '\0';

	cout << "Received from server: " << response << endl;

	if (strcmp(response, "DuplicatedName") == 0) {
		cout << "Name already exists. Please choose a different name: ";
		cin.getline(ClientName, sizeof(ClientName));

		// �ٽ� �̸��� ������ ����
		send(ServerSocket, ClientName, strlen(ClientName), 0);
	}
	else {
		cout << "Connected successfully. You can start sending messages." << endl;
	}


	/*
	// �ߺ� �̸� �޽����� ������ ��� �� �̸��� ��û
	if (strcmp(response, "DuplicateName") == 0) {
		// cout << "Name already exists. Please choose a different name: ";
		cin.getline(ClientName, sizeof(ClientName));

		// �ٽ� �̸��� ������ ����
		send(ServerSocket, ClientName, strlen(ClientName), 0);
	}
	else
	{
		cout << "Connected sucessfully. You can start sending Messages." << endl;
	}
	*/
	// ������ ���� �� ReceiveMessages �Լ� ����
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReceiveMessages, (LPVOID)ServerSocket, 0, NULL);


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

	CloseHandle(hThread);
	closesocket(ServerSocket);
	WSACleanup();

	return 0;
}