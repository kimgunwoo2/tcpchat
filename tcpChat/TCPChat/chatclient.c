#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>    // client side program
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h> 
#include <time.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

unsigned WINAPI SendMsg(void * arg);
unsigned WINAPI RecvMsg(void * arg);
void ErrorHandling(char * msg);
int recvn(SOCKET s, char *buf, int len, int flags);
char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];
char nickname[20] = "";
int main(int argc, char *argv[])
{
	WSADATA wsaData;
	SOCKET hSock;
	SOCKADDR_IN servAdr;
	HANDLE hSndThread, hRcvThread;
	if (argc != 4) {
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	}
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");
	strcpy(nickname, argv[3]);
	sprintf(name, "[%s]", argv[3]);
	hSock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr(argv[1]);
	servAdr.sin_port = htons(atoi(argv[2]));

	if (connect(hSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("connect() error");

	hSndThread =
		(HANDLE)_beginthreadex(NULL, 0, SendMsg, (void*)&hSock, 0, NULL);
	hRcvThread =
		(HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&hSock, 0, NULL);

	WaitForSingleObject(hSndThread, INFINITE);
	WaitForSingleObject(hRcvThread, INFINITE);
	closesocket(hSock);
	WSACleanup();

	return 0;
}

unsigned WINAPI SendMsg(void * arg)   // send thread main
{
	char temp[BUF_SIZE];
	char filename[BUF_SIZE];
	SOCKET hSock = *((SOCKET*)arg);
	char nameMsg[NAME_SIZE + BUF_SIZE];
	sprintf(nameMsg, "%s", nickname);
	send(hSock, nameMsg, strlen(name), 0);
	while (1) {
		fgets(msg, BUF_SIZE, stdin);
		strcpy(temp, msg);
		if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n")) {
			closesocket(hSock);
			exit(0);
		}
		else if (strcmp(strtok(temp, " ,\n"), "/fileto") == 0) {
			strcpy(filename, strtok(NULL, " ,\n"));
			strcpy(filename, strtok(NULL, " ,\n"));
			FILE *fp = fopen(filename, "rb");

			if (fp == NULL) {
				printf("전송할 파일 없음.\n");

			}
			else {
				fseek(fp, 0, SEEK_END);      // go to the end of file
				int totalbytes = ftell(fp);  // get the current position
				sprintf(nameMsg, "%s %s", name, msg);
				send(hSock, nameMsg, strlen(nameMsg), 0);
				send(hSock, (char *)&totalbytes, sizeof(totalbytes), 0);
				char buf[BUF_SIZE];
				int numread;
				int retval;
				int numtotal = 0;
				// 파일 데이터 보내기
				rewind(fp); // 파일 포인터를 제일 앞으로 이동
				while (1) {
					numread = fread(buf, 1, BUF_SIZE, fp);
					if (numread > 0) {
						retval = send(hSock, buf, numread, 0);
						if (retval == SOCKET_ERROR) {

							break;
						}
						numtotal += numread;
					}
					else if (numread == 0 && numtotal == totalbytes) {
						break;
					}
					else {
						perror("파일 입출력 오류");
						break;
					}
				}
				fclose(fp);
			}
		}
		else {
			sprintf(nameMsg, "%s %s", name, msg);
			send(hSock, nameMsg, strlen(nameMsg), 0);
		}
	}
	return 0;
}

unsigned WINAPI RecvMsg(void * arg)   // read thread main
{
	int hSock = *((SOCKET*)arg);
	char nameMsg[NAME_SIZE + BUF_SIZE];
	char temp[NAME_SIZE + BUF_SIZE];
	char buf[BUF_SIZE];
	int totalbytes;
	int retval = 0;
	int strLen;
	while (1) {
		strLen = recv(hSock, nameMsg, NAME_SIZE + BUF_SIZE - 1, 0);
		if (strLen == 0)
			break;
		nameMsg[strLen] = '\0';
		//strcpy(temp, nameMsg);
		memcpy(temp, nameMsg, strLen);
		if (strcmp(strtok(temp, " ,\n"), "/fileto") == 0) {
			strcpy(temp, strtok(NULL, " ,\n"));
			recvn(hSock, (char *)&totalbytes, sizeof(totalbytes), 0);
			FILE *fp = fopen(temp, "wb");
			int numtotal = 0;
			while (1) {
				retval = recv(hSock, buf, BUF_SIZE, 0);
				numtotal += retval;
				fwrite(buf, 1, retval, fp);
				if (numtotal == totalbytes) {
					printf("-> 파일 받기 완료!\n");
					break;
				}
			}
			fclose(fp);

		}
		else {
			fputs(nameMsg, stdout);
		}
	}
	return 0;
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