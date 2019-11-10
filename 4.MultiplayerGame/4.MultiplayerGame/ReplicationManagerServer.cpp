#include "Networks.h"
#include "ReplicationManagerServer.h"

void ReplicationManagerServer::create(uint32 networkId)
{
	replicationCommands[networkId] = ReplicationAction::Create;
}

void ReplicationManagerServer::update(uint32 networkId)
{
	replicationCommands[networkId] = ReplicationAction::Update;
}

void ReplicationManagerServer::destroy(uint32 networkId)
{
	replicationCommands[networkId] = ReplicationAction::Destroy;
}

void ReplicationManagerServer::write(OutputMemoryStream& packet)
{

	for (auto& it : replicationCommands) {
		if (it.second != ReplicationAction::None) {

			GameObject* g = App->modLinkingContext->getNetworkGameObject(it.first);
			if (g == nullptr) {
				int i = 0;
			}
			packet << it.first;
			int actionId = int(it.second);
			packet << actionId; // store the action if it is not "none"
			if (it.second == ReplicationAction::Create) {
				packet << g->position;
				packet << g->angle;
			}
			else if (it.second == ReplicationAction::Update) {
				packet << g->position;
				packet << g->angle;
			}		
		}
	}
	replicationCommands.clear();
}
