#include "Networks.h"
#include "ReplicationManagerServer.h"

void ReplicationManagerServer::create(uint32 networkId)
{
	replicationCommands[networkId] = ReplicationAction::Create;
}

void ReplicationManagerServer::update(uint32 networkId){
	if(replicationCommands[networkId] == ReplicationAction::None) //gives priority to create or destroy before update 
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
			packet << it.first;
			int actionId = int(it.second);
			packet << actionId; // store the action if it is not "none"
			if (it.second == ReplicationAction::Create) {
				packet << g->position;
				packet << g->angle;
				packet << g->size;
				std::string textureDir = std::string(g->texture->filename);
				packet << textureDir;
			}
			else if (it.second == ReplicationAction::Update) {
				packet << g->position;
				packet << g->angle;
			}				
			else if (it.second == ReplicationAction::Destroy) {
				//int i = 20;
			}
		}
	}
	replicationCommands.clear();

	
}

void ReplicationManagerServer::AppendLostCommands(std::unordered_map<uint32, ReplicationAction>* repCommands)
{
	for (auto it : *repCommands) {
		if (replicationCommands.find(it.first) == replicationCommands.end()) {
			replicationCommands[it.first] = it.second;
		}
		else {
			if (replicationCommands[it.first] == ReplicationAction::Update) {//only override if the new replication command (not the not send) is in action update
				replicationCommands[it.first] = it.second;
			}
		}
	}	
}

ReplicationDeliveryDelegate::ReplicationDeliveryDelegate(std::unordered_map<uint32, ReplicationAction>& map, ReplicationManagerServer* server)
{
	storedReplicationCommands = map;
	serverReplicationManager = server;
}

void ReplicationDeliveryDelegate::onDeliverySuccess(DeliveryManager * deliveryManager)
{
	//succesfully delivered replication delivery

}

void ReplicationDeliveryDelegate::onDeliveryFailure(DeliveryManager * deliveryManager)
{
	//resend packet
	serverReplicationManager->AppendLostCommands(&storedReplicationCommands);
	
}