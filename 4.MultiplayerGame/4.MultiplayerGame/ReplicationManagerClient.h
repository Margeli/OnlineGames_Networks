#pragma once

class ReplicationManagerClient {
public:

	struct CreatePacket{
		uint32 networkID;
		vec2 position;
		float angle;
		vec2 size;
		std::string texName;
	};
	struct UpdatePacket {
		uint32 networkID;
		vec2 position;
		float angle;
	};
	struct DestroyPacket {
		uint32 networkID;
	};

	void read(const InputMemoryStream &packet, uint32 clientID);

	std::vector<CreatePacket> packetsToCrate;
	std::vector<UpdatePacket> packetsToUpdate;
	std::vector<DestroyPacket> packetsToDestroy;

};