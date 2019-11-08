#include "Networks.h"
#include "ReplicationManagerClient.h"

void ReplicationManagerClient::read(const InputMemoryStream & packet)
{
	while (packet.RemainingByteCount() > 0) {
		uint32 networkID;
		packet >> networkID;		
		ReplicationAction action;
		packet >> action;
		if (action == ReplicationAction::Create) {
			GameObject* g = Instantiate();
			App->modLinkingContext->registerNetworkGameObjectWithNetworkId(g, networkID);
			packet >> g->position;
			packet >> g->angle;
		}
		else if (action == ReplicationAction::Update) {
			GameObject* g = App->modLinkingContext->getNetworkGameObject(networkID);
			packet >> g->position;
			packet >> g->angle;
		}
		else if (action == ReplicationAction::Destroy) {
			GameObject* g = App->modLinkingContext->getNetworkGameObject(networkID);
			App->modLinkingContext->unregisterNetworkGameObject(g);
			Destroy(g);
		}
	}
}
