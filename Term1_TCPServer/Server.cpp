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
#define IP_STRING_LENGTH 16 // IPv4 주소 문자열의 최대 길이.. 라고 하더라

// MYSQL 서버 연결 정보
const string server = "tcp://127.0.0.1:3306";
const string username = "chanynotnerd";
const string password = "rlacksdud721";

const string database = "test";
const string tableName = "User";

sql::Driver* driver;	// MYSQL DB와의 연결 설정을 위함
sql::Connection* con;	// MYSQL DB와의 연결을 나타냄, driver를 사용하기 위해 db서버와의 연결 설정 및 상호작용 위함.
sql::Statement* stmt;	// SQL 쿼리 실행 위함. DDL(데이터 정의어) 용도
sql::PreparedStatement* pstmt;	// SQL 쿼리 실행 위함. DML(데이터 조작어) 용도 
// 마지막 DCL(데이터 제어어)은 권한 부여 같은 GRANT, REVOKE가 들어간다.
sql::ResultSet* result;	// 데이터 작업 결과 저장 및 조회를 ResultSet에서 함. 

void ErrorHandling(const char* message);	// 어느 부분에서 에러 났는지 확인하기 위함
unsigned int __stdcall HandleClient(void* arg);	// 다중 클라이언트 사용을 위함

// WinSock 관련 사용해야 하는 것들 미리 선언
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


	// Winsock 초기화, 소켓 사용하기 전에 꼭 초기화 해야함, 그냥 첫 순서
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	// 소켓 생성, 
	hServSock = socket(PF_INET, SOCK_STREAM, 0);

	if (hServSock == INVALID_SOCKET)
		ErrorHandling("socket() error!");

	// 메모리 시작점부터 특정 값으로 지정하고 싶다면 사용, 여기선 초기화?
	memset(&servAddr, 0, sizeof(servAddr));

	// 각 포트에 해당하는 값 넣어주기
	servAddr.sin_family = AF_INET;	// 주소체계 
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);  // INADDY_ANY는 컴퓨터에 존재하는 랜카드 중 사용 가능한 랜카드 사용하세요 라는 뜻
	servAddr.sin_port = htons(port);	// port 지정(7777)

	// bind() / 소켓의 ip주소와 port 할당
	if (_WINSOCK2API_::bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
		ErrorHandling("bind() error!");

	// listen() / 클라이언트들의 요청을 대기시킴.
	if (listen(hServSock, 5) == SOCKET_ERROR)    // 소켓 지정 번호
		ErrorHandling("listen() error!");

	// 연결 수락, 여러 클라이언트를 받기 위해 While문으로 들어갈 예정
	/* szClntAddr = sizeof(clntAddr);
	hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &szClntAddr);  
	*/

	// 클라이언트 소켓을 관리하기 위한 배열
	SOCKET clientSockets[FD_SETSIZE];
	int numClients = 0;

	sql::Connection* con = SetupMySQLConnection();  // MySQL 연결 설정

	while (1)
	{
		// 클라이언트의 요청 확인, listen 함수의 값을 가져와 연결소켓 생성해줌
		szClntAddr = sizeof(clntAddr);
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &szClntAddr);

		if (hClntSock == INVALID_SOCKET)
		{
			ErrorHandling("accept() error!");
			continue;  // 에러가 발생하면 다음 클라이언트를 받음(FIFO 구조).
		}

		// 클라이언트들 소켓 배열에 추가하기.
		if (numClients < FD_SETSIZE)
		{
			clientSockets[numClients++] = hClntSock;
		}

		// 클라이언트마다 스레드 생성
		unsigned int threadID;
		// 스레드 생성 후 HandleClient 함수를 실행
		// HandleClient 함수는 hClntSock를 인수로 받아 클라이언트와 통신.
		// 멀티스레딩 프로그래밍 시 process.h 사용, _beginthreadex()는 스레드 생성 및 제어하기 위한 함수.
		_beginthreadex(NULL, 0, HandleClient, (void*)hClntSock, 0, &threadID);
	}

	// 클라이언트 종료
	for (int i = 0; i < numClients; i++)
	{
		closesocket(clientSockets[i]);
	}

	closesocket(hServSock);
	WSACleanup();

	delete pstmt;	// 동적 할당 메모리 pstmt 해제(메모리 로스 방지)
	delete con;		// 동적 할당 메모리 con 해제(메모리 로스 방지)
	system("pause");

	return 0;
}

void PerformMySQLOperations(const char* ClientName, SOCKET hClntSock) {
	sql::Connection* con = SetupMySQLConnection();

	try {
		// 이름 중복 확인 및 저장 로직
		sql::PreparedStatement* pstmt;
		pstmt = con->prepareStatement("SELECT COUNT(*) FROM " + tableName + " WHERE name = ?");
		pstmt->setString(1, ClientName);

		sql::ResultSet* res = pstmt->executeQuery();
		if (res->next() && res->getInt(1) == 0) {
			// 중복되지 않는 이름이라면 데이터베이스에 저장
			pstmt = con->prepareStatement("INSERT INTO " + tableName + " (name) VALUES (?)");
			pstmt->setString(1, ClientName);
			pstmt->execute();
		}
		else {
			// printf("Name already exists. Please choose a different name.\n");
			const char* duplicateNameMsg = "DuplicateName";
			send(hClntSock, duplicateNameMsg, strlen(duplicateNameMsg), 0);

			// 클라이언트로부터 새 이름을 요청
			const char* newNameRequestMsg = "Please enter a new name: ";
			send(hClntSock, newNameRequestMsg, strlen(newNameRequestMsg), 0);

			char buffer[1024];
			int recvlen = recv(hClntSock, buffer, sizeof(buffer), 0);
			if (recvlen > 0) {
				buffer[recvlen] = '\0';
				ClientName = buffer;

				// 새 이름으로 다시 중복 확인 및 저장
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

// 클라이언트 이름이 중복인지 확인하는 함수
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

	// 클라이언트 정보 가져오기
	char clntIP[IP_STRING_LENGTH];	// 얘 때문에 #include < Ws2tcpip.h > 사용
	char ClientName[100]; // 클라이언트의 이름을 저장할 변수

	// 클라이언트의 이름 입력 받기
	int recvlen = recv(hClntSock, ClientName, sizeof(ClientName), 0);
	if (recvlen > 0)
	{
		ClientName[recvlen] = '\0';
		// MySQL 작업 함수 호출
		if (IsNameDuplicate(ClientName)) {
			// 중복 이름인 경우 클라이언트에 중복 메시지를 보냄
			const char* duplicateNameMsg = "DuplicateName";
			send(hClntSock, duplicateNameMsg, strlen(duplicateNameMsg), 0);

			// 새 이름을 클라이언트로부터 다시 받음
			char newNameBuffer[1024];
			int newNameLen = recv(hClntSock, newNameBuffer, sizeof(newNameBuffer), 0);
			if (newNameLen > 0) {
				newNameBuffer[newNameLen] = '\0';

				// 새 이름으로 다시 중복 확인 및 저장
				while (IsNameDuplicate(newNameBuffer)) {
					const char* duplicateNameMsg = "DuplicateName";
					send(hClntSock, duplicateNameMsg, strlen(duplicateNameMsg), 0);

					// 새 이름을 클라이언트로부터 다시 받음
					newNameLen = recv(hClntSock, newNameBuffer, sizeof(newNameBuffer), 0);
					if (newNameLen > 0) {
						newNameBuffer[newNameLen] = '\0';
					}
				}

				strcpy_s(ClientName, sizeof(ClientName), newNameBuffer);
			}
		}

		printf("%s 님이 접속하셨습니다. \n", ClientName);
		PerformMySQLOperations(ClientName, hClntSock);
		// printf("Client %s:%d's name is %s.\n", clntIP, ntohs(clntAddr.sin_port), ClientName);
	}

	if (getpeername(hClntSock, (SOCKADDR*)&clntAddr, &clntAddrLen) == 0)	// 소켓이 연결된 peer의 주소 확인. hClntSock이 소켓 지시자이며 여기에 연결한 상대의 주소 정보를 가져옴.
	{
		if (inet_ntop(AF_INET, &(clntAddr.sin_addr), clntIP, IP_STRING_LENGTH) != NULL)	// 주소 정보를 가져와 연결이 되었으면 서버의 콘솔창에 연결된 클라이언트의 정보 출력
		{
			// printf("Connecting client %s:%d\n", clntIP, ntohs(clntAddr.sin_port));
			
		}
		else
		{
			perror("inet_ntop");	// 오류 메세지 출력.
		}
	}

	while (1)
	{
		int recvlen = recv(hClntSock, buffer, sizeof(buffer), 0);
		if (recvlen > 0)
		{
			// 클라이언트의 응답 출력
			buffer[recvlen] = '\0';
			// printf("Received from client %s:%d: %s\n", clntIP, ntohs(clntAddr.sin_port), buffer);
			printf("%s 님의 대화 : %s\n", ClientName, buffer);

		}
		else if (recvlen == 0)
		{
			// 클라이언트가 quit으로 연결을 끊었을 경우
			// printf("Client%s:%d disconnected.\n", clntIP, ntohs(clntAddr.sin_port));
			printf("%s님이 quit 채팅을 통해 대화방을 나가셨습니다.\n", ClientName);
			break;
		}
		else
		{
			// 예외 : quit으로 안 끄고 x 눌러 콘솔창 종료, 이외에도 있겠지만 우선 보이는게 이거라서.. 
			printf("%s님이 강제종료를 하고 나가셨습니다.\n", ClientName);
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