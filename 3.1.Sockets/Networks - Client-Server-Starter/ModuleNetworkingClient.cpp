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
		
		ImGui::Separator();
		ImGui::BeginChild("------------------CHAT------------------");
		for (auto &it : chat) {
			ImVec4 colorTxt = ImVec4(1, 1, 1, 1);
			switch (it.color) {
			case 0:
				colorTxt = ImVec4(1, 1, 1, 1);//white
				break;
			case 1:
				colorTxt = ImVec4(1, 0,0, 1);//red
				break;
			case 2:
				colorTxt = ImVec4(0,0, 1, 1);//blue
				break;
			case 3:
				colorTxt = ImVec4(1, 1, 0, 1);//yellow
				break;
			case 4:
				colorTxt = ImVec4(0, 1, 0, 1);//green
				break;
			}
			ImGui::TextColored(colorTxt, it.txt.c_str());
		}

		
		ImGui::InputText("", chatTxt, IM_ARRAYSIZE(chatTxt) );
		ImGui::SameLine();
		if (ImGui::Button("Send")) {
			if (!isCommand(chatTxt)) {

				OutputMemoryStream packet;
				packet << ClientMessage::SendChatMsg;
				packet << playerName;
				packet << chatTxt;
				if (sendPacket(packet, _socket)) {

					memset(chatTxt, 0, IM_ARRAYSIZE(chatTxt));
				}
				else {
					ELOG("sending chat message.");
					reportError("sending chat message.");
				}
			}
		}
		ImGui::EndChild();
		ImGui::End();
	}
	
	

	return true;
}

void ModuleNetworkingClient::sendToChat(const char * txt, int color) 
{
	ChatMsg msg = ChatMsg(txt, color);
	chat.push_back(msg);
}
void ModuleNetworkingClient::clearChat() 
{
	chat.clear();
}

bool ModuleNetworkingClient::isCommand(const char *txt)
{
	if (txt[0] == '/') {
		if (strcmp(txt, HELP_COMMAND) == 0) {
			// do help
			OutputMemoryStream packet;
			packet << ClientMessage::RequestHelpCommand;
			if (!sendPacket(packet, _socket)) {
				ELOG("Error showing help command message.");
				reportError("showing help command message.");
			}
		}
		else {
			sendToChat("Unknown command, type /help.", 1);//red
		}
	}
	else
		return false;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, const InputMemoryStream &packet)
{
	
	ServerMessage serverMessage;
	packet >> serverMessage;
	switch (serverMessage) {
		case ServerMessage::Welcome: {
			std::string msg = std::string();
			packet >> msg;
			int color;
			packet >> color;
			bool colorRed = true;
			packet >> colorRed;
			LOG(msg.c_str());
			sendToChat(msg.c_str(), color);
			break; 
		}
		case ServerMessage::UserNameExists: {
			std::string msg = std::string();
			packet >> msg;
			ELOG(msg.c_str());
			onSocketDisconnected(_socket);
			state = ClientState::Stopped;
			break;
		}
		case ServerMessage::UserLeft: {
			std::string msg = std::string();
			packet >> msg; 
			int color;
			packet >> color;
			sendToChat(msg.c_str(),color);
			LOG(msg.c_str());
			break;
		}
		case ServerMessage::UserJoin: {
			std::string msg = std::string();
			packet >> msg; 
			int color;
			packet >> color;
			sendToChat(msg.c_str(), color);
			LOG(msg.c_str());
			break;
		}
		case ServerMessage::ReceiveChatMsg: {
			std::string emitter = std::string();
			packet >> emitter;
			char txt[MAX_CHAR_INPUT_CHAT];
			memset(txt, 0, IM_ARRAYSIZE(txt));
			
			packet >> txt; 
			int color;
			packet >> color;
			std::string newtext = std::string(emitter + " says: " + txt);
			sendToChat(newtext.c_str(),color);
			break;
		}
		case ServerMessage::ShowHelpCommand: {
			std::string txt = std::string();
			packet >> txt;
			int color;
			packet >> color;
			sendToChat(txt.c_str(), color);
			break;
		}
	}
	
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
	shutdown(socket, 2);	
	clearChat();
}

