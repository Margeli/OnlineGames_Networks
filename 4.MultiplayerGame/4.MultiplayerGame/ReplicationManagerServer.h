#pragma once

enum class ReplicationAction {
	None,
	Create,
	Update,
	Destroy
};

struct ReplicationCommand {
	ReplicationAction action;
	uint32 networkID;
};

class ReplicationManagerServer {

	public:
		void create(uint32 networkId);
		void update(uint32 networkId);
		void destroy(uint32 networkId);

		void write(OutputMemoryStream& packet);

		void AppendLostCommands(std::unordered_map<uint32, ReplicationAction> *repCommands);
		
		std::unordered_map<uint32, ReplicationAction> replicationCommands;
};

class ReplicationDeliveryDelegate : public DeliveryDelegate {
public:
	ReplicationDeliveryDelegate(std::unordered_map<uint32, ReplicationAction> & repCommands, ReplicationManagerServer* server);

	void onDeliverySuccess(DeliveryManager* deliveryManager) override;
	void onDeliveryFailure(DeliveryManager* deliveryManager) override;

	std::unordered_map<uint32, ReplicationAction> storedReplicationCommands;
	ReplicationManagerServer* serverReplicationManager;
};