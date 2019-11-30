#include "Networks.h"
#include "ModuleNetworkingServer.h"



//////////////////////////////////////////////////////////////////////
// ModuleNetworkingServer public methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::setListenPort(int port)
{
	listenPort = port;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::onStart()
{
	if (!createSocket()) return;

	// Reuse address
	int enable = 1;
	int res = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));
	if (res == SOCKET_ERROR) {
		reportError("ModuleNetworkingServer::start() - setsockopt");
		disconnect();
		return;
	}

	// Create and bind to local address
	if (!bindSocketToPort(listenPort)) {
		return;
	}

	state = ServerState::Listening;

	secondsSinceLastPing = 0.0f;
}

void ModuleNetworkingServer::onGui()
{
	if (ImGui::CollapsingHeader("ModuleNetworkingServer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Connection checking info:");
		ImGui::Text(" - Ping interval (s): %f", PING_INTERVAL_SECONDS);
		ImGui::Text(" - Disconnection timeout (s): %f", DISCONNECT_TIMEOUT_SECONDS);

		ImGui::Separator();

		ImGui::Text("Replication");
		ImGui::InputFloat("Delivery interval (s)", &replicationDeliveryIntervalSeconds, 0.01f, 0.1f);
		
		ImGui::Separator();

		if (state == ServerState::Listening)
		{
			int count = 0;

			for (int i = 0; i < MAX_CLIENTS; ++i)
			{
				if (clientProxies[i].name != "")
				{
					ImGui::Text("CLIENT %d", count++);
					ImGui::Text(" - address: %d.%d.%d.%d",
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b1,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b2,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b3,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b4);
					ImGui::Text(" - port: %d", ntohs(clientProxies[i].address.sin_port));
					ImGui::Text(" - name: %s", clientProxies[i].name.c_str());
					ImGui::Text(" - id: %d", clientProxies[i].clientId);
					ImGui::Text(" - Last packet time: %.04f", clientProxies[i].lastPacketReceivedTime);
					ImGui::Text(" - Seconds since repl.: %.04f", clientProxies[i].secondsSinceLastReplication); 
					ImGui::Text("Pending Deliveries to Ack: %i", clientProxies[i].deliveryManagerServer.countPendingDeliveries());

					
					ImGui::Separator();
				}
			}

			ImGui::Checkbox("Render colliders", &App->modRender->mustRenderColliders);
		}
	}
}

void ModuleNetworkingServer::onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress)
{
	if (state == ServerState::Listening)
	{
		// Register player
		ClientProxy *proxy = getClientProxy(fromAddress);

		// Read the packet type
		ClientMessage message;
		packet >> message;

		// Process the packet depending on its type
		if (message == ClientMessage::Hello)
		{
			bool newClient = false;

			if (proxy == nullptr && !runningGame)
			{
				proxy = createClientProxy();

				newClient = true;
				//process the sequence number to the hello message delivery
				if (proxy->deliveryManagerServer.processSequenceNumber(packet)) {
					std::string playerName;
					uint8 spaceshipType;
					packet >> playerName;
					packet >> spaceshipType;

					proxy->address.sin_family = fromAddress.sin_family;
					proxy->address.sin_addr.S_un.S_addr = fromAddress.sin_addr.S_un.S_addr;
					proxy->address.sin_port = fromAddress.sin_port;
					proxy->connected = true;
					proxy->name = playerName;
					proxy->clientId = nextClientId++;

					// Create new network object
					spawnPlayer(*proxy, spaceshipType);

					// Send welcome to the new player
					OutputMemoryStream welcomePacket;
					welcomePacket << ServerMessage::Welcome;
					welcomePacket << proxy->clientId;
					welcomePacket << proxy->gameObject->networkId;
					sendPacket(welcomePacket, fromAddress);

					// Send all network objects to the new player
					uint16 networkGameObjectsCount;
					GameObject *networkGameObjects[MAX_NETWORK_OBJECTS];
					App->modLinkingContext->getNetworkGameObjects(networkGameObjects, &networkGameObjectsCount);
					for (uint16 i = 0; i < networkGameObjectsCount; ++i)
					{
						GameObject *gameObject = networkGameObjects[i];

						// TO_DO(jesus): Notify the new client proxy's replication manager about the creation of this game object
						proxy->replicationManagerServer.create(gameObject->networkId);
					}

					LOG("Message received: hello - from player %s", playerName.c_str());
				}
			}

			if (!newClient)
			{
				// Send welcome to the new player
				OutputMemoryStream unwelcomePacket;
				unwelcomePacket << ServerMessage::Unwelcome;
				if (runningGame) {
					unwelcomePacket << DisconnectionError::gameRunning;
				}
				else {
					unwelcomePacket << DisconnectionError::network;
				}
				
				sendPacket(unwelcomePacket, fromAddress);

				WLOG("Message received: UNWELCOMED hello.");
			}
		}
		else if (message == ClientMessage::Input)
		{
			// Process the input packet and update the corresponding game object
			if (proxy != nullptr)
			{
				// Read input data
				while (packet.RemainingByteCount() > 0)
				{
					InputPacketData inputData;
					packet >> inputData.sequenceNumber;
					packet >> inputData.horizontalAxis;
					packet >> inputData.verticalAxis;
					packet >> inputData.buttonBits;

					//get the packet's last sequence number and send it to the client 
					//in a replication packet

					if (inputData.sequenceNumber >= proxy->nextExpectedInputSequenceNumber)
					{
						proxy->gamepad.horizontalAxis = inputData.horizontalAxis;
						proxy->gamepad.verticalAxis = inputData.verticalAxis;
						unpackInputControllerButtons(inputData.buttonBits, proxy->gamepad);
						if(proxy->gameObject->state==GameObject::State::UPDATING && proxy->gameObject->active)
							proxy->gameObject->behaviour->onInput(proxy->gamepad);
						proxy->nextExpectedInputSequenceNumber = inputData.sequenceNumber + 1;
						proxy->lastInputSequenceNumberReceived = inputData.sequenceNumber;
					}
				}				
			}
		}

		else if (message == ClientMessage::ReplicationAck) {			

			proxy->deliveryManagerServer.processAckdSequenceNumbers(packet);
		}
		else if (message == ClientMessage::ReadyToPlay) {
			proxy->readyToPlay = true;
			proxy->deliveryManagerServer.processSequenceNumber(packet);
		}

		if (proxy != nullptr)
		{
			proxy->lastPacketReceivedTime = Time.time;
		}
	}
}

void ModuleNetworkingServer::onUpdate()
{
	if (state == ServerState::Listening)
	{

		if (!runningGame && CheckAllPlayersReady()) {
			runningGame = true;
			GameStart();
		}
		if (runningGame) {
			GameUpdate();
		}

		bool sendPing = false;
		secondsSinceLastPing += Time.deltaTime;
		if (secondsSinceLastPing > PING_INTERVAL_SECONDS) {
			sendPing = true;
			secondsSinceLastPing = 0;
		}

		// Replication
		for (ClientProxy& clientProxy : clientProxies)
		{
			if (clientProxy.connected)
			{
				///////////////////////////////////////TIMEOUT
				if (Time.time - clientProxy.lastPacketReceivedTime > DISCONNECT_TIMEOUT_SECONDS) {
					ELOG("client connection timeout with the server.");
					NetworkDestroy(clientProxy.gameObject);
					destroyClientProxy(&clientProxy); 
					
					break;
				}
				///////////////////////////////////////-TIMEOUT

				///////////////////////////////////////PING
				if (sendPing) { //server PING to every client
					OutputMemoryStream pingPacket;
					pingPacket << ServerMessage::Ping;
					if (clientProxy.deliveryManagerServer.hasSequenceNumberPendingAck()) {
						clientProxy.deliveryManagerServer.writeSequenceNumbersPendingAck(pingPacket);//sending sequence number Ack in the ping packet

					}
					sendPacket(pingPacket, clientProxy.address);

				}
				///////////////////////////////////////-PING

				///////////////////////////////////////REPLICATION
				
				// TO_DO(jesus): If the replication interval passed and the replication manager of this proxy
				//              has pending data, write and send a replication packet to this client.
				clientProxy.secondsSinceLastReplication += Time.deltaTime;
				if (clientProxy.secondsSinceLastReplication > replicationDeliveryIntervalSeconds) {
					
						OutputMemoryStream RepPacket;
						RepPacket << ServerMessage::Replication;
						//replication message containing the input sequence number
						uint32 num = clientProxy.lastInputSequenceNumberReceived;
						RepPacket << num;

						//game
						RepPacket << gameTimer;


						//writing the replication sequence number
						
						ReplicationDeliveryDelegate* delegate = nullptr;
						if (clientProxy.replicationManagerServer.replicationCommands.size() > 0)
							delegate = new ReplicationDeliveryDelegate(clientProxy.replicationManagerServer.replicationCommands, &clientProxy.replicationManagerServer);
						clientProxy.deliveryManagerServer.writeSequenceNumber(RepPacket, *delegate);
						

						//writing the replication data
						if (clientProxy.replicationManagerServer.replicationCommands.size() > 0) {
							clientProxy.replicationManagerServer.write(RepPacket );
						}
						clientProxy.secondsSinceLastReplication = 0;
						sendPacket(RepPacket, clientProxy.address);
				}
				///////////////////////////////////////-REPLICATION


				

				///////////////////////////////////////DELIVERY ACKNOWLEDGEMENT
				
				clientProxy.deliveryManagerServer.processTimedOutPackets();
				///////////////////////////////////////-DELIVERY ACKNOWLEDGEMENT

				}
		}
	}
}

void ModuleNetworkingServer::onConnectionReset(const sockaddr_in & fromAddress)
{
	// Find the client proxy
	ClientProxy *proxy = getClientProxy(fromAddress);
	
	if (proxy)
	{
		// Notify game object deletion to replication managers
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			if (clientProxies[i].connected && proxy->clientId != clientProxies[i].clientId)
			{
				// TO_DO(jesus): Notify this proxy's replication manager about the destruction of this player's game object
				clientProxies[i].replicationManagerServer.destroy(proxy->gameObject->networkId);
			}
		}

		// Unregister the network identity
		App->modLinkingContext->unregisterNetworkGameObject(proxy->gameObject);

		// Remove its associated game object
		Destroy(proxy->gameObject);

		// Clear the client proxy
		destroyClientProxy(proxy);
	}
}

void ModuleNetworkingServer::onDisconnect()
{
	// Destroy network game objects
	uint16 netGameObjectsCount;
	GameObject *netGameObjects[MAX_NETWORK_OBJECTS];
	App->modLinkingContext->getNetworkGameObjects(netGameObjects, &netGameObjectsCount);
	for (uint32 i = 0; i < netGameObjectsCount; ++i)
	{
		NetworkDestroy(netGameObjects[i]);
	}

	// Clear all client proxies
	for (ClientProxy &clientProxy : clientProxies)
	{
		destroyClientProxy(&clientProxy);
	}
	
	nextClientId = 0;

	state = ServerState::Stopped;
}


//////////////////////////////////////////////////////////////////////
// Game logic
//////////////////////////////////////////////////////////////////////
void ModuleNetworkingServer::GameStart() {

	
}
void ModuleNetworkingServer::GameUpdate()
{
	gameTimer += Time.deltaTime;

	currAsteroidsSpawnTime += Time.deltaTime;
	if (currAsteroidsSpawnTime > asteroidsSpawnTime) {
		currAsteroidsSpawnTime = 0.0f;
		spawnAsteroid();
	}

	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (clientProxies[i].connected) {
			//whent the spaceship of the player dies
			if(clientProxies[i].notifyDead) {
				OutputMemoryStream packet;
				packet << ServerMessage::EndGame;
				packet << gameTimer;
				packet << currentPlayers;
				currentPlayers--;
				sendPacket(packet, clientProxies[i].address);

				clientProxies[i].notifyDead = false;
			}
		}
	}
	if (currentPlayers == 0) {
		//restart game
	}
}

bool ModuleNetworkingServer::CheckAllPlayersReady()
{
	bool ret = false;
	bool ready = true;
	currentPlayers = 0;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (clientProxies[i].connected) {
			ret = true;
			ready &= clientProxies[i].readyToPlay;
			currentPlayers++;
		}
	}
	return ret & ready;
}



//////////////////////////////////////////////////////////////////////
// Client proxies
//////////////////////////////////////////////////////////////////////

ModuleNetworkingServer::ClientProxy * ModuleNetworkingServer::getClientProxy(const sockaddr_in &clientAddress)
{
	// Try to find the client
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].address.sin_addr.S_un.S_addr == clientAddress.sin_addr.S_un.S_addr &&
			clientProxies[i].address.sin_port == clientAddress.sin_port)
		{
			return &clientProxies[i];
		}
	}

	return nullptr;
}

ModuleNetworkingServer::ClientProxy * ModuleNetworkingServer::getClientProxyByGO(const GameObject * g)
{
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].gameObject == g)
		{
			return &clientProxies[i];
		}
	}
	return nullptr;
}

ModuleNetworkingServer::ClientProxy * ModuleNetworkingServer::createClientProxy()
{
	// If it does not exist, pick an empty entry
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!clientProxies[i].connected)
		{
			return &clientProxies[i];
		}
	}

	return nullptr;
}

void ModuleNetworkingServer::destroyClientProxy(ClientProxy * proxy)
{
	*proxy = {};
}



//////////////////////////////////////////////////////////////////////
// Spawning
//////////////////////////////////////////////////////////////////////

GameObject * ModuleNetworkingServer::spawnPlayer(ClientProxy &clientProxy, uint8 spaceshipType)
{
	// Create a new game object with the player properties
	clientProxy.gameObject = Instantiate();
	clientProxy.gameObject->size = { 100, 100 };
	clientProxy.gameObject->angle = 45.0f;

	if (spaceshipType == 0) {
		clientProxy.gameObject->texture = App->modResources->spacecraft1;
	}
	else if (spaceshipType == 1) {
		clientProxy.gameObject->texture = App->modResources->spacecraft2;
	}
	else if (spaceshipType == 2) {
		clientProxy.gameObject->texture = App->modResources->spacecraft3;
	}
	else if (spaceshipType == 3) {
		clientProxy.gameObject->texture = App->modResources->spacecraft4;
	}
	else if (spaceshipType == 4) {
		clientProxy.gameObject->texture = App->modResources->spacecraft5;
	}
	else {
		clientProxy.gameObject->texture = App->modResources->spacecraft6;
	}

	// Create collider
	clientProxy.gameObject->collider = App->modCollision->addCollider(ColliderType::Player, clientProxy.gameObject);
	clientProxy.gameObject->collider->isTrigger = true;

	// Create behaviour
	clientProxy.gameObject->behaviour = new Spaceship;
	clientProxy.gameObject->behaviour->gameObject = clientProxy.gameObject;

	// Assign a new network identity to the object
	App->modLinkingContext->registerNetworkGameObject(clientProxy.gameObject);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TO_DO(jesus): Notify this proxy's replication manager about the creation of this game object
			clientProxies[i].replicationManagerServer.create(clientProxy.gameObject->networkId);
		}
	}

	return clientProxy.gameObject;
}

GameObject * ModuleNetworkingServer::spawnBullet(GameObject *parent)
{
	// Create a new game object with the player properties
	GameObject *gameObject = Instantiate();
	gameObject->size = { 20, 60 };
	gameObject->angle = parent->angle;
	gameObject->position = parent->position;
	gameObject->texture = App->modResources->laser;
	gameObject->collider = App->modCollision->addCollider(ColliderType::Laser, gameObject);

	// Create behaviour
	gameObject->behaviour = new Laser;
	gameObject->behaviour->gameObject = gameObject;

	// Assign a new network identity to the object
	App->modLinkingContext->registerNetworkGameObject(gameObject);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TO_DO(jesus): Notify this proxy's replication manager about the creation of this game object
			clientProxies[i].replicationManagerServer.create(gameObject->networkId);
		}
	}

	return gameObject;
}

GameObject * ModuleNetworkingServer::spawnAsteroid()
{
	GameObject* g = Instantiate();

	//random size
	float size = Random.randomFloat(50, 100);
	g->size = { size, size };

	//random initial pos
	g->position = { Random.randomFloat(440,490), Random.randomFloat(-350, 350) };
	g->texture =App->modResources->asteroid;
	g->collider = App->modCollision->addCollider(ColliderType::Asteroid, g);

	g->behaviour = new Asteroid;
	g->behaviour->gameObject = g;
	

	//every GAME_ASTEROIDS_DIFFICULTY_RATIO seconds (20s) the new asteroids speed is doubled
	Asteroid* beh = (Asteroid*)g->behaviour;
	beh->speed = Random.randomFloat(75, 125) * Time.time / GAME_ASTEROIDS_DIFFICULTY_RATIO ;


	App->modLinkingContext->registerNetworkGameObject(g);

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			clientProxies[i].replicationManagerServer.create(g->networkId);
		}
	}

	return g;
}


//////////////////////////////////////////////////////////////////////
// Update / destruction
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::destroyNetworkObject(GameObject * gameObject)
{
	// Notify all client proxies' replication manager to destroy the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TO_DO(jesus): Notify this proxy's replication manager about the destruction of this game object
			clientProxies[i].replicationManagerServer.destroy(gameObject->networkId);
		}
	}

	// Assuming the message was received, unregister the network identity
	App->modLinkingContext->unregisterNetworkGameObject(gameObject);

	// Finally, destroy the object from the server
	Destroy(gameObject);
}

void ModuleNetworkingServer::updateNetworkObject(GameObject * gameObject)
{
	// Notify all client proxies' replication manager to destroy the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TO_DO(jesus): Notify this proxy's replication manager about the update of this game object
			clientProxies[i].replicationManagerServer.update(gameObject->networkId);
		}
	}
}




//////////////////////////////////////////////////////////////////////
// Global update / destruction of game objects
//////////////////////////////////////////////////////////////////////

void NetworkUpdate(GameObject * gameObject)
{
	ASSERT(App->modNetServer->isConnected());

	App->modNetServer->updateNetworkObject(gameObject);
}

void NetworkDestroy(GameObject * gameObject)
{
	ASSERT(App->modNetServer->isConnected());

	App->modNetServer->destroyNetworkObject(gameObject);
}
