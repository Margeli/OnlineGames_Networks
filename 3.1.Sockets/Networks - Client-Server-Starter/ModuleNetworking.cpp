#include "Networks.h"
#include "ModuleNetworking.h"


static uint8 NumModulesUsingWinsock = 0;



void ModuleNetworking::reportError(const char* inOperationDesc)
{
	LPVOID lpMsgBuf;
	DWORD errorNum = WSAGetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	ELOG("Error %s: %d- %s", inOperationDesc, errorNum, lpMsgBuf);
}

void ModuleNetworking::disconnect()
{
	for (SOCKET socket : sockets)
	{
		shutdown(socket, 2);
		closesocket(socket);
	}

	sockets.clear();
}

bool ModuleNetworking::init()
{
	if (NumModulesUsingWinsock == 0)
	{
		NumModulesUsingWinsock++;

		WORD version = MAKEWORD(2, 2);
		WSADATA data;
		if (WSAStartup(version, &data) != 0)
		{
			reportError("ModuleNetworking::init() - WSAStartup");
			return false;
		}
	}

	return true;
}

bool ModuleNetworking::preUpdate()
{
	if (sockets.empty()) return true;

	// NOTE(jesus): You can use this temporary buffer to store data from recv()
	const uint32 incomingDataBufferSize = Kilobytes(1);

	// TODO(jesus): select those sockets that have a read operation available
	fd_set	readSet;
	FD_ZERO(&readSet);
	for (auto i : sockets) {
		FD_SET(i, &readSet);
	}

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int iResult = select(0,  &readSet,nullptr, nullptr, &timeout);
	if (iResult == SOCKET_ERROR) {
		reportError("error selecting sockets for reading");
	}

	// TODO(jesus): for those sockets selected, check wheter or not they are
	// a listen socket or a standard socket and perform the corresponding
	// operation (accept() an incoming connection or recv() incoming data,
	// respectively).
	// On accept() success, communicate the new connected socket to the
	// subclass (use the callback onSocketConnected()), and add the new
	// connected socket to the managed list of sockets.
	// On recv() success, communicate the incoming data received to the
	// subclass (use the callback onSocketReceivedData()).


	for (auto it : sockets) {
		if (FD_ISSET(it, &readSet)){
			if (App->modNetServer->isListenSocket(it)) { // if it is server socket

				SOCKET newSocket = INVALID_SOCKET;
				sockaddr_in adrsBound;
				int size = sizeof(adrsBound);

				newSocket = accept(it, (sockaddr*)&adrsBound, &size);
				if (newSocket == INVALID_SOCKET) {
					reportError("error accepting socket ");
				}
				onSocketConnected(newSocket, adrsBound);
				addSocket(newSocket);
			}
			else { // if standard socket
				InputMemoryStream packet;
				int bytesRead = recv(it,packet.GetBufferPtr(), packet.GetCapacity(), 0);
				if (bytesRead == 0 || bytesRead == SOCKET_ERROR || bytesRead == ECONNRESET) {
					if (bytesRead != 0) {
						reportError("Error receiving socket");
					}
					disconnectedSockets.push_back(it);
				}
				else {
					packet.SetSize((uint32)bytesRead);
					onSocketReceivedData(it, packet);
				}
			}
		}
	}
	for (auto it : disconnectedSockets) {
		onSocketDisconnected(it);
		int i = 0;
		for (auto it2 : sockets) {
			
			if (it == it2) {

				sockets.erase(sockets.begin() + i);
				
				break;
			}
			++i;
		}
	}
	disconnectedSockets.clear();
	// TODO(jesus): handle disconnections. Remember that a socket has been
	// disconnected from its remote end either when recv() returned 0,
	// or when it generated some errors such as ECONNRESET.
	// Communicate detected disconnections to the subclass using the callback
	// onSocketDisconnected().

	// TODO(jesus): Finally, remove all disconnected sockets from the list
	// of managed sockets.

	return true;
}

bool ModuleNetworking::cleanUp()
{
	disconnect();

	NumModulesUsingWinsock--;
	if (NumModulesUsingWinsock == 0)
	{

		if (WSACleanup() != 0)
		{
			reportError("ModuleNetworking::cleanUp() - WSACleanup");
			return false;
		}
	}

	return true;
}

void ModuleNetworking::addSocket(SOCKET socket)
{
	sockets.push_back(socket);
}

bool ModuleNetworking::sendPacket(const OutputMemoryStream &packet, SOCKET socket) 
{
	int result = send(socket, packet.GetBufferPtr(), packet.GetSize(), 0);
	if (result == SOCKET_ERROR) {
		reportError("send");
		return false;
	}
	return true;
}