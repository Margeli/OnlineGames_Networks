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
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);//init WinSock Lib
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Client error initializing Winsock Lib");
	}

	// TODO-2: Create socket (IPv4, stream, TCP)
	
	SOCKET socky = socket(AF_INET, SOCK_STREAM, 0); // IPv4, TCP socket
	if (socky == INVALID_SOCKET) {
		printWSErrorAndExit("Client error creating socket, INVALID_SOCKET");
	}
	// TODO-3: Create an address object with the server address

	sockaddr_in addressBound;
	addressBound.sin_family = AF_INET; //IPv4
	addressBound.sin_port = htons(port);
	inet_pton(AF_INET, serverAddrStr, &addressBound.sin_addr);

	// TODO-4: Connect to server
	iResult = connect(socky, (sockaddr*)&addressBound, sizeof(addressBound)); ///EXLUSIVE OF TCP SOCKETS (listen, accept, connect)
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Client error connecting to server");
	}

	for (int i = 0; i < 5; ++i)
	{
		// TODO-5:
		// - Send a 'ping' packet to the server
		// - Receive 'pong' packet from the server
		// - Control errors in both cases
		// - Control graceful disconnection from the server (recv receiving 0 bytes)
		std::string ping = std::string("ping");
		iResult = send(socky, ping.c_str(), ping.length()+1, 0);
		if (iResult == SOCKET_ERROR) {
			printWSErrorAndExit("Client error sending message");
		}

		char recv_msg[16];
		iResult = recv(socky, recv_msg, 16, 0);
		if (iResult == SOCKET_ERROR) {
			printWSErrorAndExit("Client error receiving message");
		}
		if (sizeof(recv_msg) == 0) {
			printWSErrorAndExit("Client error lost connection to server");
		}
		std::cout << recv_msg << std::endl;
		Sleep(500);
	}

	// TODO-6: Close socket
	iResult = closesocket(socky);
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Client error closing socket");
	}

	// TODO-7: Winsock shutdown
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
