#include "Networks.h"
#include "ReplicationManagerClient.h"

void ReplicationManagerClient::read(const InputMemoryStream & packet, uint32 clientID)
{
	uint32 networkID;
	while (packet.RemainingByteCount() > 0) {
		packet >> networkID;
		int actionId = 0;
		packet >> actionId;
		ReplicationAction action = ReplicationAction(actionId);

		switch (action) {
		case ReplicationAction::Create: {
			CreatePacket cPack;
			packet >> cPack.position;
			packet >> cPack.angle;
			packet >> cPack.size;
			packet >> cPack.texName;
			cPack.networkID = networkID;
			packetsToCrate.push_back(cPack);
			break;
		}
		case ReplicationAction::Update: {
			UpdatePacket uPack;
			packet >> uPack.position;
			packet >> uPack.angle;
			uPack.networkID = networkID;
			packetsToUpdate.push_back(uPack);
			break;
		}
		case ReplicationAction::Destroy: {
			DestroyPacket dPack;
			dPack.networkID = networkID;
			packetsToDestroy.push_back(dPack);
			break;
		}
		}
	}

	for (auto &it : packetsToDestroy) {
		GameObject* g = App->modLinkingContext->getNetworkGameObject(it.networkID);
		if (g != nullptr) {
			App->modLinkingContext->unregisterNetworkGameObject(g);
			Destroy(g);
		}
		else {
			ELOG("network ID object not found %i", it.networkID);
		}
	}
	packetsToDestroy.clear();

	for (auto &it : packetsToCrate) {
		GameObject* g = Instantiate();
		App->modLinkingContext->registerNetworkGameObjectWithNetworkId(g, it.networkID);
	
		if (networkID == clientID)
		{
			Spaceship* behavior = new Spaceship();
			behavior->gameObject = g;
			g->behaviour = behavior;
		}
		g->position = it.position;
		g->angle = it.angle;
		g->size = it.size;
		g->texture = App->modTextures->loadTexture(it.texName.c_str());
	}
	packetsToCrate.clear();

	for (auto &it : packetsToUpdate) {
		GameObject* g = App->modLinkingContext->getNetworkGameObject(it.networkID);
		if (g != nullptr) {
			g->position = it.position;
			g->angle = it.angle;
		}
		else {
			ELOG("network ID object not found %i", it.networkID);
		}
	}
	packetsToUpdate.clear();
		
}
