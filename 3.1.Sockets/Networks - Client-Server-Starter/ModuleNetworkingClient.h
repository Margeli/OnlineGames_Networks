#pragma once

#include "ModuleNetworking.h"
class ModuleNetworkingClient : public ModuleNetworking
{
public:
	struct ChatMsg {

		ChatMsg(std::string _chat, int _color) : txt(_chat), color(_color) {};		
		std::string txt;
		int color = 0; //0 white, 1 red, 2 blue
	};
	//////////////////////////////////////////////////////////////////////
	// ModuleNetworkingClient public methods
	//////////////////////////////////////////////////////////////////////

	bool start(const char *serverAddress, int serverPort, const char *playerName);

	bool isRunning() const;



private:

	//////////////////////////////////////////////////////////////////////
	// Module virtual methods
	//////////////////////////////////////////////////////////////////////

	bool update() override;

	bool gui() override;

	void sendToChat(const char* txt, int color);

	void clearChat();

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworking virtual methods
	//////////////////////////////////////////////////////////////////////

	void onSocketReceivedData(SOCKET socket, const InputMemoryStream &packet) override;

	void onSocketDisconnected(SOCKET socket) override;



	//////////////////////////////////////////////////////////////////////
	// Client state
	//////////////////////////////////////////////////////////////////////

	enum class ClientState
	{
		Stopped,
		Start,
		Logging
	};

	ClientState state = ClientState::Stopped;

	sockaddr_in serverAddress = {};
	SOCKET _socket = INVALID_SOCKET;

	std::vector<ChatMsg> chat;

	std::string playerName;
	char chatTxt[MAX_CHAR_INPUT_CHAT];
};

