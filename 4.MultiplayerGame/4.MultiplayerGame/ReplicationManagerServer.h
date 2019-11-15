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

		
		std::unordered_map<uint32, ReplicationAction> replicationCommands;
};