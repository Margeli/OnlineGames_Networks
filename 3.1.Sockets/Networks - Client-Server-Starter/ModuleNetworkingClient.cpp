#include "ModuleNetworkingClient.h"


bool  ModuleNetworkingClient::start(const char * serverAddressStr, int serverPort, const char *pplayerName)
{
	playerName = pplayerName;

	// TODO(jesus): TCP connection stuff
	// - Create the socket
	_socket = socket(AF_INET, SOCK_STREAM, 0); // IPv4, TCP socket
	if (_socket == INVALID_SOCKET) {
		reportError("Client error creating socket, INVALID_SOCKET");
	}

	// - Create the remote address object
	sockaddr_in addressBound;
	addressBound.sin_family = AF_INET; //IPv4
	addressBound.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverAddressStr, &addressBound.sin_addr);

	// - Connect to the remote address
	int iResult = connect(_socket, (sockaddr*)&addressBound, sizeof(addressBound)); ///EXLUSIVE OF TCP SOCKETS (listen, accept, connect)
	if (iResult != NO_ERROR) {//error case	
		reportError("Client error connecting to server");
	}	

	// - Add the created socket to the managed list of sockets using addSocket()
	addSocket(_socket);
	// If everything was ok... change the state
	state = ClientState::Start;

	return true;
}

bool ModuleNetworkingClient::isRunning() const
{
	return state != ClientState::Stopped;
}

bool ModuleNetworkingClient::update()
{
	if (state == ClientState::Start)
	{
		OutputMemoryStream packet;
		packet << ClientMessage::Hello;
		packet << playerName;
		if (sendPacket(packet, _socket)) {

			state = ClientState::Logging;
		}
		else {
			disconnect();
			state = ClientState::Stopped;
		}
	}

	return true;
}

bool ModuleNetworkingClient::gui()
{
	if (state != ClientState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Client Window");

		Texture *tex = App->modResources->client;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		if (ImGui::Button("Disconnect Client")) {
			onSocketDisconnected(_socket);
		}

		ImGui::Text("%s connected to the server...", playerName.c_str());

		ImGui::End();
	}

	return true;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, const InputMemoryStream &packet)
{
	state = ClientState::Stopped;

}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
	shutdown(socket, 2);	
	
}

