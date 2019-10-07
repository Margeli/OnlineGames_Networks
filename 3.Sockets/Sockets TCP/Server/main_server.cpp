#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "Windows.h"
#include "WinSock2.h"
#include "Ws2tcpip.h"

#include <iostream>
#include <cstdlib>

#pragma comment(lib, "ws2_32.lib")

#define LISTEN_PORT 8888

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

	// TODO-2: Create socket (IPv4, stream, TCP)

	SOCKET socky = socket(AF_INET, SOCK_STREAM, 0); // IPv4, UDP socket
	if (socky == INVALID_SOCKET) {
		printWSErrorAndExit("Server error creating socket, INVALID_SOCKET");
	}

	// TODO-3: Configure socket for address reuse

	int enable = 1;
	iResult = setsockopt(socky, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Server error reusing socket's address");
	}
	// TODO-4: Create an address object with any local address

	sockaddr_in addressBound;
	addressBound.sin_family = AF_INET; //IPv4
	addressBound.sin_port = htons(port);		
	addressBound.sin_addr.S_un.S_addr = INADDR_ANY;

	

	// TODO-5: Bind socket to the local address

	iResult = bind(socky, (const sockaddr*)&addressBound, sizeof(addressBound));
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Server error binding socket's address");
	}

	// TODO-6: Make the socket enter into listen mode
	iResult = listen(socky, 3);
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Server error listening socket");
	}

	// TODO-7: Accept a new incoming connection from a remote host
	// Note that once a new connection is accepted, we will have
	// a new socket directly connected to the remote host.
	int size = sizeof(addressBound);
	SOCKET acceptedSocky = accept(socky, (sockaddr*)&addressBound, &size);
	if (acceptedSocky = INVALID_SOCKET) {
		printWSErrorAndExit("Server error accepting socket");
	}
	int counter = 0;
	while (counter<5)
	{
		// TODO-8:
		// - Wait a 'ping' packet from the client
		// - Send a 'pong' packet to the client
		// - Control errors in both cases

		char recv_msg[16];
		iResult = recv(acceptedSocky, recv_msg, 16, 0);
		if (iResult == SOCKET_ERROR) {
			printWSErrorAndExit("Client error receiving message");
			delete[] recv_msg;
		}

		std::string pong = std::string("pong");
		iResult = send(acceptedSocky, pong.c_str(), pong.length(), 0);
		if (iResult == SOCKET_ERROR) {
			printWSErrorAndExit("Server error sending message");
		}
		std::cout << recv_msg << std::endl;
		delete[] recv_msg;
		counter++;
	}

	// TODO-9: Close socket
	iResult = closesocket(acceptedSocky);
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Client error closing socket");
	}
	iResult = closesocket(socky);
	if (iResult != NO_ERROR) {//error case	
		printWSErrorAndExit("Client error closing socket");
	}
	// TODO-10: Winsock shutdown
		iResult = WSACleanup();
		if (iResult != NO_ERROR) {//error case	
			printWSErrorAndExit("Client error shutting down Winsock Lib");
		}
}

int main(int argc, char **argv)
{
	server(LISTEN_PORT);

	PAUSE_AND_EXIT();
}
