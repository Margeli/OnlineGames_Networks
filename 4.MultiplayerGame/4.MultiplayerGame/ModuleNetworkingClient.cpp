#include "Networks.h"
#include "ModuleNetworkingClient.h"



//////////////////////////////////////////////////////////////////////
// ModuleNetworkingClient public methods
//////////////////////////////////////////////////////////////////////


void ModuleNetworkingClient::setServerAddress(const char * pServerAddress, uint16 pServerPort)
{
	serverAddressStr = pServerAddress;
	serverPort = pServerPort;
}

void ModuleNetworkingClient::setPlayerInfo(const char * pPlayerName, uint8 pSpaceshipType)
{
	playerName = pPlayerName;
	spaceshipType = pSpaceshipType;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingClient::onStart()
{
	if (!createSocket()) return;

	if (!bindSocketToPort(0)) {
		disconnect();
		return;
	}

	// Create remote address
	serverAddress = {};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(serverPort);
	int res = inet_pton(AF_INET, serverAddressStr.c_str(), &serverAddress.sin_addr);
	if (res == SOCKET_ERROR) {
		reportError("ModuleNetworkingClient::startClient() - inet_pton");
		disconnect();
		return;
	}

	state = ClientState::Start;

	inputDataFront = 0;
	inputDataBack = 0;

	secondsSinceLastInputDelivery = 0.0f;
	secondsSinceLastPing = 0.0f;
	lastPacketReceivedTime = Time.time;
}

void ModuleNetworkingClient::onGui()
{
	if (state == ClientState::Stopped) return;

	if (ImGui::CollapsingHeader("ModuleNetworkingClient", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (state == ClientState::WaitingWelcome)
		{
			ImGui::Text("Waiting for server response...");
		}
		else if (state == ClientState::Playing)
		{
			ImGui::Text("Connected to server");

			ImGui::Separator();

			ImGui::Text("Player info:");
			ImGui::Text(" - Id: %u", playerId);
			ImGui::Text(" - Name: %s", playerName.c_str());

			ImGui::Separator();

			ImGui::Text("Spaceship info:");
			ImGui::Text(" - Type: %u", spaceshipType);
			ImGui::Text(" - Network id: %u", networkId);

			vec2 playerPosition = {};
			float angle = 0;
			GameObject *playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
			if (playerGameObject != nullptr) {
				playerPosition = playerGameObject->position;
				angle = playerGameObject->angle;
			}
			ImGui::Text(" - Coordinates: (%f, %f)", playerPosition.x, playerPosition.y);
			ImGui::Text(" - Angle: %f", angle);//DELETE-------------------------

			ImGui::Separator();

			ImGui::Text("Connection checking info:");
			ImGui::Text(" - Ping interval (s): %f", PING_INTERVAL_SECONDS);
			ImGui::Text(" - Disconnection timeout (s): %f", DISCONNECT_TIMEOUT_SECONDS);
			ImGui::Text("Pending Deliveries to Ack: %i", deliveryManagerClient.countPendingDeliveries());
			ImGui::Separator();

			ImGui::Text("Input:");
			ImGui::InputFloat("Delivery interval (s)", &inputDeliveryIntervalSeconds, 0.01f, 0.1f, 4);

			ImGui::Separator();
			
		}
	}
	ImGui::Begin("GAME");
	ImGui::Text("Time: %.1f", serverGameTime);
	if (!readyToPlay) {
		if (ImGui::Button("Start Game", { ImGui::GetWindowWidth(),20 })) {
			sendReadyPacket = true;
		}
	}
	else {
		if (deadInGame) {
			ImGui::Text("You last: %.1f s", deadTime);
			if (positionInGame == 1) {
				ImGui::Text("YOU WIN!");
				ImGui::Text("WARNING! The server will restart in %i sec.", GAME_END_RESTART_SERVER_TIME);
			}
			else {
				ImGui::Text("You lose, position #%i", positionInGame);
			}
		}
		else {
			ImGui::Text("Ready to play");
		}
	}
	ImGui::End();
}

void ModuleNetworkingClient::onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress)
{
	lastPacketReceivedTime = Time.time;

	ServerMessage message;
	packet >> message;
	if (message == ServerMessage::Ping) {
		deliveryManagerClient.processAckdSequenceNumbers(packet);

	}

	if (state == ClientState::WaitingWelcome)
	{
		if (message == ServerMessage::Welcome)
		{
			packet >> playerId;
			packet >> networkId;

			LOG("ModuleNetworkingClient::onPacketReceived() - Welcome from server");
			state = ClientState::Playing;
		}
		else if (message == ServerMessage::Unwelcome)
		{
			DisconnectionError error;
			packet >> error;
			if (error == DisconnectionError::gameRunning) {
				WLOG("Game already running. Wait until it finishes");
			}
			WLOG("ModuleNetworkingClient::onPacketReceived() - Unwelcome from server :-(");
			disconnect();
		}
	}
	else if (state == ClientState::Playing)
	{
		// TO_DO(jesus): Handle incoming messages from server
		if (message == ServerMessage::Replication) {

			//replication message containing the input sequence number
			uint32 lastInputSequenceNum=0;
			packet >> lastInputSequenceNum;

			//game
			
			packet >> serverGameTime;

			//replication sequence number
			if (deliveryManagerClient.processSequenceNumber(packet)) {
				//if the replication sequence number is the correct

				//reading the replication data
				replicationManagerClient.read(packet, networkId);
			}

			//lastInputSequenceNumberReceivedByServer = lastInputSequenceNum;


			//////////////////////////////////////////RECONCILITATION
				GameObject* playerClientGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
				if (playerClientGameObject && lastInputSequenceNum > lastInputSequenceNumberReceivedByServer) {
					InputController inputForServer;
					for (uint32 i = lastInputSequenceNumberReceivedByServer; i < lastInputSequenceNum; ++i)
					{
						InputPacketData &inputPacketData = inputData[i % ArrayCount(inputData)];
						inputControllerFromInputPacketData(inputPacketData, inputForServer);
						if(playerClientGameObject->behaviour)
							playerClientGameObject->behaviour->onInput(inputForServer);
						
					}
					lastInputSequenceNumberReceivedByServer = lastInputSequenceNum;
				}
			//////////////////////////////////////////-RECONCILIATION
		}
		if (message == ServerMessage::PlayerDead) {
			deadInGame = true;
			packet >> deadTime;
			packet >> positionInGame;
		}
		if (message == ServerMessage::EndGame) {
			RestartClientGame();
		}
	}
}

void ModuleNetworkingClient::onUpdate()
{
	if (state == ClientState::Stopped) return;

	if (state == ClientState::Start)
	{
		// Send the hello packet with player data

		sendHelloPacket();

		state = ClientState::WaitingWelcome;
	}
	else if (state == ClientState::WaitingWelcome)
	{
	}
	else if (state == ClientState::Playing)
	{
		//////////////////////////////////////////READY TO PLAY MSG
		if (sendReadyPacket) {
			OutputMemoryStream packet;
			packet << ClientMessage::ReadyToPlay;

			ReadyToPlayDeliveryDelegate* delegate = nullptr;
			delegate = new ReadyToPlayDeliveryDelegate(this);
			deliveryManagerClient.writeSequenceNumber(packet, *delegate);
			
			sendPacket(packet, serverAddress);
			sendReadyPacket = false;
		}
		//////////////////////////////////////////-READY TO PLAY MSG

		//////////////////////////////////////////TIMEOUT
		if (Time.time - lastPacketReceivedTime > DISCONNECT_TIMEOUT_SECONDS) {
			ELOG("client connection timeout with the server.");
			onDisconnect();
			disconnect();
		}
		////////////////////////////////////////////-TIMEOUT

		//////////////////////////////////////////CLIENT PREDICTION
			GameObject *playerClientGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
			if (playerClientGameObject != nullptr&& playerClientGameObject->behaviour!=nullptr)
			{
				playerClientGameObject->behaviour->onInput(Input);
			}
		//////////////////////////////////////////-CLIENT PREDICTION


		/////////////////////////////////////////PING
		secondsSinceLastPing += Time.deltaTime;
		if (secondsSinceLastPing > PING_INTERVAL_SECONDS) //sends ping every PING_INTERVAL_SECONDS
			sendPing();
		/////////////////////////////////////////-PING

		//////////////////////////////////////////INPUT
		secondsSinceLastInputDelivery += Time.deltaTime;

		inputDataFront = lastInputSequenceNumberReceivedByServer;//

		if (inputDataBack - inputDataFront < ArrayCount(inputData))
		{
			uint32 currentInputData = inputDataBack++;
			InputPacketData &inputPacketData = inputData[currentInputData % ArrayCount(inputData)];
			inputPacketData.sequenceNumber = currentInputData;
			inputPacketData.horizontalAxis = Input.horizontalAxis;
			inputPacketData.verticalAxis = Input.verticalAxis;
			inputPacketData.buttonBits = packInputControllerButtons(Input);
			

			// Create packet (if there's input and the input delivery interval exceeded)
			if (secondsSinceLastInputDelivery > inputDeliveryIntervalSeconds)
			{
				secondsSinceLastInputDelivery = 0.0f;

				OutputMemoryStream packet;
				packet << ClientMessage::Input;

				for (uint32 i = inputDataFront; i < inputDataBack; ++i)
				{
					InputPacketData &inputPacketData = inputData[i % ArrayCount(inputData)];
					packet << inputPacketData.sequenceNumber;
					packet << inputPacketData.horizontalAxis;
					packet << inputPacketData.verticalAxis;
					packet << inputPacketData.buttonBits;
				}

				// Clear the queue
				//inputDataFront = inputDataBack;
				
				sendPacket(packet, serverAddress);
			}
		}
		/////////////////////////////////////-INPUT

		////////////////////////////////REPLICATION PENDING ACK
		if (deliveryManagerClient.hasSequenceNumberPendingAck()) {
			OutputMemoryStream repPacket;
			repPacket << ClientMessage::ReplicationAck;
			deliveryManagerClient.writeSequenceNumbersPendingAck(repPacket);

			sendPacket(repPacket, serverAddress);
		}

		//////////////////////////////////-REPLICATION PENDING ACK

	}

	// Make the camera focus the player game object
	GameObject *playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
	if (playerGameObject != nullptr)
	{
		App->modRender->cameraPosition = playerGameObject->position;
	}
}

void ModuleNetworkingClient::onConnectionReset(const sockaddr_in & fromAddress)
{
	disconnect();
}

void ModuleNetworkingClient::onDisconnect()
{
	state = ClientState::Stopped;

	// Get all network objects and clear the linking context
	uint16 networkGameObjectsCount;
	GameObject *networkGameObjects[MAX_NETWORK_OBJECTS] = {};
	App->modLinkingContext->getNetworkGameObjects(networkGameObjects, &networkGameObjectsCount);
	App->modLinkingContext->clear();

	// Destroy all network objects
	for (uint32 i = 0; i < networkGameObjectsCount; ++i)
	{
		Destroy(networkGameObjects[i]);
	}

	App->modRender->cameraPosition = {};
}

void ModuleNetworkingClient::sendPing()
{
	OutputMemoryStream pingPacket;
	pingPacket << ClientMessage::Ping;
	sendPacket(pingPacket, serverAddress);
	secondsSinceLastPing = 0.0f;
}

void ModuleNetworkingClient::RestartClientGame()
{
	readyToPlay = false;
	sendReadyPacket = false;
	deadInGame = false;
	serverGameTime = 0.0f;
	deadTime = 0.0f;
	positionInGame = 0;
	//state = ClientState::Start;
	deliveryManagerClient.restart();
	
	disconnect();
}

void ModuleNetworkingClient::sendHelloPacket()
{
	OutputMemoryStream stream;
	stream << ClientMessage::Hello;

	LoginDeliveryDelegate* delegate = nullptr;
	delegate = new LoginDeliveryDelegate(this);
	deliveryManagerClient.writeSequenceNumber(stream, *delegate);

	stream << playerName;
	stream << spaceshipType;


	sendPacket(stream, serverAddress);
}

void LoginDeliveryDelegate::onDeliverySuccess(DeliveryManager * deliveryManager)
{
	//succesfully delivered login delivery
	
}

void LoginDeliveryDelegate::onDeliveryFailure(DeliveryManager * deliveryManager)
{
	networkingClient->sendHelloPacket();
}

void ReadyToPlayDeliveryDelegate::onDeliverySuccess(DeliveryManager * deliveryManager)
{
	networkingClient->readyToPlay = true;
}

void ReadyToPlayDeliveryDelegate::onDeliveryFailure(DeliveryManager * deliveryManager)
{
	networkingClient->sendReadyPacket = true;
}
