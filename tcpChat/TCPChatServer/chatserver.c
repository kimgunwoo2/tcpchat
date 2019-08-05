#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h> 

#define BUF_SIZE 100
#define MAX_CLNT 256
typedef struct  ClientList
{
	char nickname[20];
	char ipaddress[20];
}ClientList;
unsigned WINAPI HandleClnt(void * arg);
void SendMsg(char * msg, int len);
void ErrorHandling(char * msg);
int recvn(SOCKET s, char *buf, int len, int flags);

int clntCnt = 0;
SOCKET clntSocks[MAX_CLNT];
HANDLE hMutex;
ClientList clientls[20];

int main(int argc, char *argv[])
{
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;
	int clntAdrSz;
	HANDLE  hThread;
	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");
	hMutex = CreateMutex(NULL, FALSE, NULL);
	hServSock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");
	if (listen(hServSock, 5) == SOCKET_ERROR)
		ErrorHandling("listen() error");

	while (1) {
		clntAdrSz = sizeof(clntAdr);
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &clntAdrSz);

		WaitForSingleObject(hMutex, INFINITE);
		clntSocks[clntCnt++] = hClntSock;


		char nickname[20] = "";
		recv(hClntSock, nickname, sizeof(nickname), 0);
		printf("nick : %s\n", nickname);
		strcpy(clientls[clntCnt - 1].nickname, nickname);
		strcpy(clientls[clntCnt - 1].ipaddress, inet_ntoa(clntAdr.sin_addr));
		printf("ip : %s \n", clientls[clntCnt - 1].ipaddress);
		ReleaseMutex(hMutex);
		hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClnt,
			(void*)&hClntSock, 0, NULL);
		printf("Connected client IP: %s \n",
			inet_ntoa(clntAdr.sin_addr));
	}
	closesocket(hServSock);
	WSACleanup();
	return 0;
}
unsigned WINAPI HandleClnt(void * arg)
{
	SOCKET hClntSock = *((SOCKET*)arg);
	int strLen = 0, i;
	char* tok;

	char temp[BUF_SIZE];
	char msg[BUF_SIZE];
	char nameMsg[BUF_SIZE];
	while ((strLen = recv(hClntSock, msg, sizeof(msg), 0)) != 0) {
		if (strLen == -1) {
			break;
		}
		memcpy(temp, msg, strLen);
		tok = strtok(temp, " ,\n");
		tok = strtok(NULL, " ,\n");
		if (msg != NULL) {

		}
		if (strcmp(tok, "/list") == 0) {
			for (i = 0; i<clntCnt; i++)
			{
				if (hClntSock == clntSocks[i])
				{
					strcpy(nameMsg, "============================\n");
					send(clntSocks[i], nameMsg, strlen(nameMsg), 0);
					strcpy(nameMsg, "현재 연결되어 있는 member\n");
					send(clntSocks[i], nameMsg, strlen(nameMsg), 0);
					for (int j = 0; j < clntCnt; j++) {
						sprintf(nameMsg, "nickname : %s ip : %s\n", clientls[j].nickname, clientls[j].ipaddress);
						send(clntSocks[i], nameMsg, strlen(nameMsg), 0);
					}
					strcpy(nameMsg, "============================\n");
					send(clntSocks[i], nameMsg, strlen(nameMsg), 0);

				}
			}
		}
		else if (strcmp(tok, "/to") == 0) {
			tok = strtok(NULL, " ,\n");
			int suc = 0;
			for (i = 0; i < clntCnt; i++)
			{
				if (strcmp(clientls[i].nickname, tok) == 0) {
					sprintf(nameMsg, "%s 님의 귓속말 : %s\n", strtok(msg, " ,\n"), strtok(NULL, "\n"));
					send(clntSocks[i], nameMsg, strlen(nameMsg), 0);
					suc = 1;
				}
			}
			if (suc == 0) {
				for (i = 0; i < clntCnt; i++)
				{
					if (hClntSock == clntSocks[i])
					{
						strcpy(nameMsg, "귓속말 할 닉네임이 없습니다.\n");
						send(clntSocks[i], nameMsg, strlen(nameMsg), 0);
					}
				}
			}

		}
		else if (strcmp(tok, "/fileto") == 0) {
			tok = strtok(NULL, " ,\n");
			int sender = 0;
			int recevie = -1;
			for (i = 0; i < clntCnt; i++)
			{
				if (hClntSock == clntSocks[i])
				{
					sender = i;
				}
			}
			for (i = 0; i < clntCnt; i++)
			{
				if (strcmp(clientls[i].nickname, tok) == 0) {
					recevie = i;

				}
			}
			tok = strtok(NULL, " ,\n");
			char filename[256];
			char buf[BUF_SIZE];
			int retval = 0;
			ZeroMemory(filename, 256);
			sprintf(filename, "/fileto %s\n", tok);
			send(clntSocks[recevie], filename, strlen(filename), 0);

			int totalbytes;
			recvn(clntSocks[sender], (char *)&totalbytes, sizeof(totalbytes), 0);
			send(clntSocks[recevie], (char *)&totalbytes, sizeof(totalbytes), 0);

			int numtotal = 0;
			while (1) {
				retval = recv(clntSocks[sender], buf, BUF_SIZE, 0);
				send(clntSocks[recevie], buf, retval, 0);
				numtotal += retval;
				if (numtotal == totalbytes) {
					if (recevie != -1)
						send(clntSocks[sender], "파일 전송 완료.\n", sizeof("파일 전송 완료.\n"), 0);
					break;
				}
			}
			if (recevie == -1) {
				send(clntSocks[sender], "파일보낼 닉네임이 없습니다.\n", sizeof("파일보낼 닉네임이 없습니다.\n"), 0);
			}


		}
		else {

			SendMsg(msg, strLen);
		}
	}
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i<clntCnt; i++)   // remove disconnected client
	{
		if (hClntSock == clntSocks[i])
		{
			while (i++ < clntCnt - 1) {
				clntSocks[i] = clntSocks[i + 1];
				strcpy(clientls[i].nickname, clientls[i + 1].nickname);
				strcpy(clientls[i].ipaddress, clientls[i + 1].ipaddress);
			}
			break;
		}
	}
	clntCnt--;
	ReleaseMutex(hMutex);
	closesocket(hClntSock);
	return 0;
}
void SendMsg(char * msg, int len)   // send to all
{
	int i;
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i<clntCnt; i++)
		send(clntSocks[i], msg, len, 0);

	ReleaseMutex(hMutex);
}

void ErrorHandling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}
	return (len - left);
}