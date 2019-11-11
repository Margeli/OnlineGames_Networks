#include "Networks.h"
#include "ReplicationManagerClient.h"

void ReplicationManagerClient::read(const InputMemoryStream & packet)
{
	while (packet.RemainingByteCount() > 0) {
		uint32 networkID;
		packet >> networkID;		
		int actionId = 0;
		packet >> actionId;
		ReplicationAction action = ReplicationAction(actionId);
		if (action == ReplicationAction::Create) {
			GameObject* g = Instantiate();
			App->modLinkingContext->registerNetworkGameObjectWithNetworkId(g, networkID);
			packet >> g->position;
			packet >> g->angle;
			packet >> g->size;
			std::string textureDir = std::string();
			packet >> textureDir;
			g->texture = App->modTextures->loadTexture(textureDir.c_str());
			
			/*int type = 0;
			packet >> type;			
			switch (type) {
				case 0: {
					g->texture = App->modResources->spacecraft1;
					break; 
				}
				case 1: {
					g->texture = App->modResources->spacecraft2;
					break;
				}
				case 3: {
					g->texture = App->modResources->spacecraft3;
					break;
				}
			}*/
		}
		else if (action == ReplicationAction::Update) {
			GameObject* g = App->modLinkingContext->getNetworkGameObject(networkID);
			if (g != nullptr) {
				packet >> g->position;
				packet >> g->angle;
			}
			else {
				ELOG("network ID object not found %i", networkID);
			}
		}
		else if (action == ReplicationAction::Destroy) {
			GameObject* g = App->modLinkingContext->getNetworkGameObject(networkID);
			App->modLinkingContext->unregisterNetworkGameObject(g);
			Destroy(g);
		}
	}
}
