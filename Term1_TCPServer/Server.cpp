#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <process.h>
#include <Ws2tcpip.h>


#pragma comment(lib, "ws2_32.lib")
#define IP_STRING_LENGTH 16 // IPv4 �ּ� ���ڿ��� �ִ� ����.. ��� �ϴ���


void ErrorHandling(const char* message);	// ��� �κп��� ���� ������ Ȯ���ϱ� ����
unsigned int __stdcall HandleClient(void* arg);	// ���� Ŭ���̾�Ʈ ����� ����

int main()
{
	// ����ؾ� �ϴ� �͵� �̸� ����
	WSADATA     wsaData;
	SOCKET      hServSock, hClntSock;
	SOCKADDR_IN servAddr, clntAddr;

	short   port = 7777;
	int     szClntAddr;
	char    message[] = "0";
	char    buffer[1024] = { 0, };

	// Winsock �ʱ�ȭ, ���� ����ϱ� ���� �� �ʱ�ȭ �ؾ���, �׳� ù ����
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	// ���� ����, 
	hServSock = socket(PF_INET, SOCK_STREAM, 0);

	if (hServSock == INVALID_SOCKET)
		ErrorHandling("socket() error!");

	// �޸� ���������� Ư�� ������ �����ϰ� �ʹٸ� ���, ���⼱ �ʱ�ȭ?
	memset(&servAddr, 0, sizeof(servAddr));

	// �� ��Ʈ�� �ش��ϴ� �� �־��ֱ�
	servAddr.sin_family = AF_INET;	// �ּ�ü�� 
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);  // INADDY_ANY�� ��ǻ�Ϳ� �����ϴ� ��ī�� �� ��� ������ ��ī�� ����ϼ��� ��� ��
	servAddr.sin_port = htons(port);        // port ����(7777)

	// bind() / ������ ip�ּҿ� port �Ҵ�
	if (bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
		ErrorHandling("bind() error!");

	// listen() / Ŭ���̾�Ʈ���� ��û�� ����Ŵ.
	if (listen(hServSock, 5) == SOCKET_ERROR)    // ���� ���� ��ȣ
		ErrorHandling("listen() error!");

	// ���� ����, ���� Ŭ���̾�Ʈ�� �ޱ� ���� While������ �� ����
	/* szClntAddr = sizeof(clntAddr);
	hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &szClntAddr);  
	*/

	// Ŭ���̾�Ʈ ������ �����ϱ� ���� �迭
	SOCKET clientSockets[FD_SETSIZE];
	int numClients = 0;

	while (1)
	{
		// Ŭ���̾�Ʈ�� ��û Ȯ��, listen �Լ��� ���� ������ ������� ��������
		szClntAddr = sizeof(clntAddr);
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &szClntAddr);

		if (hClntSock == INVALID_SOCKET)
		{
			ErrorHandling("accept() error!");
			continue;  // ������ �߻��ϸ� ���� Ŭ���̾�Ʈ�� ����(FIFO ����).
		}

		// Ŭ���̾�Ʈ�� ���� �迭�� �߰��ϱ�.
		if (numClients < FD_SETSIZE)
		{
			clientSockets[numClients++] = hClntSock;
		}

		// Ŭ���̾�Ʈ���� ������ ����
		unsigned int threadID;
		// ������ ���� �� HandleClient �Լ��� ����
		// HandleClient �Լ��� hClntSock�� �μ��� �޾� Ŭ���̾�Ʈ�� ���.
		// ��Ƽ������ ���α׷��� �� process.h ���, _beginthreadex()�� ������ ���� �� �����ϱ� ���� �Լ�.
		_beginthreadex(NULL, 0, HandleClient, (void*)hClntSock, 0, &threadID);
	}

	// Ŭ���̾�Ʈ ����
	for (int i = 0; i < numClients; i++)
	{
		closesocket(clientSockets[i]);
	}

	closesocket(hServSock);
	WSACleanup();

	return 0;
}

unsigned int __stdcall HandleClient(void* arg)
{
	SOCKET hClntSock = (SOCKET)arg;
	char buffer[1024] = { 0 };
	SOCKADDR_IN clntAddr;
	int clntAddrLen = sizeof(clntAddr);

	// Ŭ���̾�Ʈ ���� ��������
	char clntIP[IP_STRING_LENGTH];	// �� ������ #include < Ws2tcpip.h > ���
	if (getpeername(hClntSock, (SOCKADDR*)&clntAddr, &clntAddrLen) == 0)	// ������ ����� peer�� �ּ� Ȯ��. hClntSock�� ���� �������̸� ���⿡ ������ ����� �ּ� ������ ������.
	{
		if (inet_ntop(AF_INET, &(clntAddr.sin_addr), clntIP, IP_STRING_LENGTH) != NULL)	// �ּ� ������ ������ ������ �Ǿ����� ������ �ܼ�â�� ����� Ŭ���̾�Ʈ�� ���� ���
		{
			printf("Connecting client %s:%d\n", clntIP, ntohs(clntAddr.sin_port));
		}
		else
		{
			perror("inet_ntop");	// ���� �޼��� ����̶�� �Ѵ�.
		}
	}

	while (1)
	{
		int recvlen = recv(hClntSock, buffer, sizeof(buffer), 0);
		if (recvlen > 0)
		{
			// Ŭ���̾�Ʈ�� ���� ���
			buffer[recvlen] = '\0';
			printf("Received from client %s:%d: %s\n", clntIP, ntohs(clntAddr.sin_port), buffer);

		}
		else if (recvlen == 0)
		{
			// Ŭ���̾�Ʈ�� quit���� ������ ������ ���
			printf("Client%s:%d disconnected.\n", clntIP, ntohs(clntAddr.sin_port));
			break;
		}
		else
		{
			// ���� : quit���� �� ���� x ���� �ܼ�â ����, �̿ܿ��� �ְ����� �켱 ���̴°� �̰Ŷ�.. 
			printf("client%s:%d Shutdown debug window.\n", clntIP, ntohs(clntAddr.sin_port));
			break;
		}
	}

	// closesocket(hClntSock);
	return 0;
}

void ErrorHandling(const char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}