#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "Windows.h"
#include "WinSock2.h"
#include "Ws2tcpip.h"

#include <iostream>
#include <cstdlib>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_ADDRESS "127.0.0.1"

#define SERVER_PORT 8888

#define PAUSE_AND_EXIT() system("pause"); exit(-1)

void printWSErrorAndExit(const char *msg)
{
	wchar_t *s = NULL;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&s, 0, NULL);
	fprintf(stderr, "%s: %S\n", msg, s);
	LocalFree(s);
	PAUSE_AND_EXIT();
}

void client(const char *serverAddrStr, int port)
{
	// TODO-1: Winsock init
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Client error initializing Winsock Lib");
	}

	// TODO-2: Create socket (IPv4, datagrams, UDP)
	SOCKET socky = socket(AF_INET, SOCK_DGRAM, 0); // IPv4, UDP socket
	if (socky == INVALID_SOCKET) {
		printWSErrorAndExit("Client error creating socket, INVALID_SOCKET");
	}

	// TODO-3: Create an address object with the server address

	sockaddr_in addressBound;
	addressBound.sin_family = AF_INET; //IPv4
	addressBound.sin_port = htons(port);
	inet_pton(AF_INET, serverAddrStr, &addressBound.sin_addr);

	int count = 0;
	while (count<5)
	{
		// TODO-4:
		// - Send a 'ping' packet to the server
		// - Receive 'pong' packet from the server
		// - Control errors in both cases
		std::string ping = std::string("ping");
			

		iResult = sendto(socky, ping.c_str(), ping.length()+1, 0, (const sockaddr*)&addressBound, sizeof(addressBound));
		if (iResult == SOCKET_ERROR) {
			printWSErrorAndExit("Client error sending message");
		}

		char recv_msg[16];
		int bindAddrLen = sizeof(addressBound);
		iResult = recvfrom(socky, recv_msg, 16, 0, (sockaddr*)&addressBound, &bindAddrLen);
		if (iResult == SOCKET_ERROR) {
			printWSErrorAndExit("Client error receiving message");
		}

		std::cout << recv_msg << std::endl;
		Sleep(500);
		count++;
	}

	// TODO-5: Close socket
	iResult = closesocket(socky);
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Client error closing socket");
	}

	// TODO-6: Winsock shutdown
	iResult = WSACleanup();
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Client error shutting down Winsock Lib");
	}
}

int main(int argc, char **argv)
{
	client(SERVER_ADDRESS, SERVER_PORT);

	PAUSE_AND_EXIT();
}
