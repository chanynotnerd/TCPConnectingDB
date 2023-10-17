#include <stdio.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>

#include <winsock2.h>
#include <process.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#define IP_STRING_LENGTH 16 // IPv4 �ּ� ���ڿ��� �ִ� ����.. ��� �ϴ���

// MYSQL ���� ���� ����
const string server = "tcp://127.0.0.1:3306";
const string username = "chanynotnerd";
const string password = "rlacksdud721";

const string database = "test";
const string tableName = "User";

sql::Driver* driver;	// MYSQL DB���� ���� ������ ����
sql::Connection* con;	// MYSQL DB���� ������ ��Ÿ��, driver�� ����ϱ� ���� db�������� ���� ���� �� ��ȣ�ۿ� ����.
sql::Statement* stmt;	// SQL ���� ���� ����. DDL(������ ���Ǿ�) �뵵
sql::PreparedStatement* pstmt;	// SQL ���� ���� ����. DML(������ ���۾�) �뵵 
// ������ DCL(������ �����)�� ���� �ο� ���� GRANT, REVOKE�� ����.
sql::ResultSet* result;	// ������ �۾� ��� ���� �� ��ȸ�� ResultSet���� ��. 

void ErrorHandling(const char* message);	// ��� �κп��� ���� ������ Ȯ���ϱ� ����
unsigned int __stdcall HandleClient(void* arg);	// ���� Ŭ���̾�Ʈ ����� ����

// WinSock ���� ����ؾ� �ϴ� �͵� �̸� ����
WSADATA     wsaData;
SOCKET      hServSock, hClntSock;
SOCKADDR_IN servAddr, clntAddr;

short   port = 7777;
int     szClntAddr;
char    message[] = "0";
char    buffer[1024] = { 0, };

sql::Connection* SetupMySQLConnection() {
	driver = get_driver_instance();
	con = driver->connect(server, username, password);
	con->setSchema(database);
	return con;
}

int main()
{	

	// ---------- WinSock Server ----------


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
	servAddr.sin_port = htons(port);	// port ����(7777)

	// bind() / ������ ip�ּҿ� port �Ҵ�
	if (_WINSOCK2API_::bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
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

	sql::Connection* con = SetupMySQLConnection();  // MySQL ���� ����

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

	delete pstmt;	// ���� �Ҵ� �޸� pstmt ����(�޸� �ν� ����)
	delete con;		// ���� �Ҵ� �޸� con ����(�޸� �ν� ����)
	system("pause");

	return 0;
}

void PerformMySQLOperations(const char* ClientName, SOCKET hClntSock) {
	sql::Connection* con = SetupMySQLConnection();

	try {
		// �̸� �ߺ� Ȯ�� �� ���� ����
		sql::PreparedStatement* pstmt;
		pstmt = con->prepareStatement("SELECT COUNT(*) FROM " + tableName + " WHERE name = ?");
		pstmt->setString(1, ClientName);

		sql::ResultSet* res = pstmt->executeQuery();
		if (res->next() && res->getInt(1) == 0) {
			// �ߺ����� �ʴ� �̸��̶�� �����ͺ��̽��� ����
			pstmt = con->prepareStatement("INSERT INTO " + tableName + " (name) VALUES (?)");
			pstmt->setString(1, ClientName);
			pstmt->execute();
		}
		else {
			// printf("Name already exists. Please choose a different name.\n");
			const char* duplicateNameMsg = "DuplicateName";
			send(hClntSock, duplicateNameMsg, strlen(duplicateNameMsg), 0);

			// Ŭ���̾�Ʈ�κ��� �� �̸��� ��û
			const char* newNameRequestMsg = "Please enter a new name: ";
			send(hClntSock, newNameRequestMsg, strlen(newNameRequestMsg), 0);

			char buffer[1024];
			int recvlen = recv(hClntSock, buffer, sizeof(buffer), 0);
			if (recvlen > 0) {
				buffer[recvlen] = '\0';
				ClientName = buffer;

				// �� �̸����� �ٽ� �ߺ� Ȯ�� �� ����
				PerformMySQLOperations(ClientName, hClntSock);
			}
		}

		delete pstmt;
		delete con;
	}
	catch (sql::SQLException& e) {
		cout << "SQLException: " << e.what() << endl;
	}
}

// Ŭ���̾�Ʈ �̸��� �ߺ����� Ȯ���ϴ� �Լ�
bool IsNameDuplicate(const string& name) {
	sql::Connection* con = SetupMySQLConnection();
	sql::PreparedStatement* pstmt;
	pstmt = con->prepareStatement("SELECT COUNT(*) FROM " + tableName + " WHERE name = ?");
	pstmt->setString(1, name);

	sql::ResultSet* res = pstmt->executeQuery();
	bool isDuplicate = res->next() && res->getInt(1) > 0;
	delete pstmt;
	delete con;
	return isDuplicate;
}


unsigned int __stdcall HandleClient(void* arg)
{	
	SOCKET hClntSock = (SOCKET)arg;
	char buffer[1024] = { 0 };
	SOCKADDR_IN clntAddr;
	int clntAddrLen = sizeof(clntAddr);

	// Ŭ���̾�Ʈ ���� ��������
	char clntIP[IP_STRING_LENGTH];	// �� ������ #include < Ws2tcpip.h > ���
	char ClientName[100]; // Ŭ���̾�Ʈ�� �̸��� ������ ����

	// Ŭ���̾�Ʈ�� �̸� �Է� �ޱ�
	int recvlen = recv(hClntSock, ClientName, sizeof(ClientName), 0);
	if (recvlen > 0)
	{
		ClientName[recvlen] = '\0';
		// MySQL �۾� �Լ� ȣ��
		if (IsNameDuplicate(ClientName)) {
			// �ߺ� �̸��� ��� Ŭ���̾�Ʈ�� �ߺ� �޽����� ����
			const char* duplicateNameMsg = "DuplicateName";
			send(hClntSock, duplicateNameMsg, strlen(duplicateNameMsg), 0);

			// �� �̸��� Ŭ���̾�Ʈ�κ��� �ٽ� ����
			char newNameBuffer[1024];
			int newNameLen = recv(hClntSock, newNameBuffer, sizeof(newNameBuffer), 0);
			if (newNameLen > 0) {
				newNameBuffer[newNameLen] = '\0';

				// �� �̸����� �ٽ� �ߺ� Ȯ�� �� ����
				while (IsNameDuplicate(newNameBuffer)) {
					const char* duplicateNameMsg = "DuplicateName";
					send(hClntSock, duplicateNameMsg, strlen(duplicateNameMsg), 0);

					// �� �̸��� Ŭ���̾�Ʈ�κ��� �ٽ� ����
					newNameLen = recv(hClntSock, newNameBuffer, sizeof(newNameBuffer), 0);
					if (newNameLen > 0) {
						newNameBuffer[newNameLen] = '\0';
					}
				}

				strcpy_s(ClientName, sizeof(ClientName), newNameBuffer);
			}
		}

		printf("%s ���� �����ϼ̽��ϴ�. \n", ClientName);
		PerformMySQLOperations(ClientName, hClntSock);
		// printf("Client %s:%d's name is %s.\n", clntIP, ntohs(clntAddr.sin_port), ClientName);
	}

	if (getpeername(hClntSock, (SOCKADDR*)&clntAddr, &clntAddrLen) == 0)	// ������ ����� peer�� �ּ� Ȯ��. hClntSock�� ���� �������̸� ���⿡ ������ ����� �ּ� ������ ������.
	{
		if (inet_ntop(AF_INET, &(clntAddr.sin_addr), clntIP, IP_STRING_LENGTH) != NULL)	// �ּ� ������ ������ ������ �Ǿ����� ������ �ܼ�â�� ����� Ŭ���̾�Ʈ�� ���� ���
		{
			// printf("Connecting client %s:%d\n", clntIP, ntohs(clntAddr.sin_port));
			
		}
		else
		{
			perror("inet_ntop");	// ���� �޼��� ���.
		}
	}

	while (1)
	{
		int recvlen = recv(hClntSock, buffer, sizeof(buffer), 0);
		if (recvlen > 0)
		{
			// Ŭ���̾�Ʈ�� ���� ���
			buffer[recvlen] = '\0';
			// printf("Received from client %s:%d: %s\n", clntIP, ntohs(clntAddr.sin_port), buffer);
			printf("%s ���� ��ȭ : %s\n", ClientName, buffer);

		}
		else if (recvlen == 0)
		{
			// Ŭ���̾�Ʈ�� quit���� ������ ������ ���
			// printf("Client%s:%d disconnected.\n", clntIP, ntohs(clntAddr.sin_port));
			printf("%s���� quit ä���� ���� ��ȭ���� �����̽��ϴ�.\n", ClientName);
			break;
		}
		else
		{
			// ���� : quit���� �� ���� x ���� �ܼ�â ����, �̿ܿ��� �ְ����� �켱 ���̴°� �̰Ŷ�.. 
			printf("%s���� �������Ḧ �ϰ� �����̽��ϴ�.\n", ClientName);
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