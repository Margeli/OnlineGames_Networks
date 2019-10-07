#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "Windows.h"
#include "WinSock2.h"
#include "Ws2tcpip.h"

#include <iostream>
#include <cstdlib>

#pragma comment(lib, "ws2_32.lib")

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

void server(int port)
{
	// TODO-1: Winsock init
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Server error initializing Winsock Lib");
	}

	// TODO-2: Create socket (IPv4, datagrams, UDP
	SOCKET socky = socket(AF_INET, SOCK_DGRAM, 0); // IPv4, UDP socket
	if (socky == INVALID_SOCKET) {
		printWSErrorAndExit("Server error creating socket, INVALID_SOCKET");
	}
	sockaddr_in addressBound;
	addressBound.sin_family = AF_INET; //IPv4
	addressBound.sin_port = htons(port);
	addressBound.sin_addr.S_un.S_addr = INADDR_ANY;

	// TODO-3: Force address reuse
	int enable = 1;
	iResult = setsockopt(socky, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Server error reusing socket's address");
	}

	// TODO-4: Bind to a local address
	iResult = bind(socky, (sockaddr*)&addressBound, sizeof(addressBound));
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Server error binding socket's address to IP");
	}
	int counter = 0;
	while (counter<5)
	{
		// TODO-5:
		// - Receive 'ping' packet from a remote host
		// - Answer with a 'pong' packet
		// - Control errors in both cases
	

		char* recv_msg[16];
		int bindAddrLen = sizeof(addressBound);
		iResult = recvfrom(socky, *recv_msg, 16, 0, (sockaddr*)&addressBound, &bindAddrLen);
		if (iResult == SOCKET_ERROR) {
			printWSErrorAndExit("Server error receiving message");
		}
		std::cout << recv_msg << std::endl;
		delete[] recv_msg;

		std::string pong = std::string("pong");
		iResult = sendto(socky, pong.c_str(), pong.length(), 0, (const sockaddr*)&addressBound, sizeof(addressBound));
		if (iResult == SOCKET_ERROR) {
			printWSErrorAndExit("Server error sending message");
		}
		Sleep(500);
	
		counter++;
	}

	// TODO-6: Close socket
	iResult = closesocket(socky);
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Server error closing socket");
	}

	// TODO-7: Winsock shutdown
	iResult = WSACleanup();
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Server error shutting down Winsock Lib");
	}
}

int main(int argc, char **argv)
{
	server(SERVER_PORT);

	PAUSE_AND_EXIT();
}
