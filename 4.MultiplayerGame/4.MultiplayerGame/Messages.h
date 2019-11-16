#pragma once

enum class ClientMessage
{
	Hello = 8758,
	Input,
	Ping,
	ReplicationAck
};

enum class ServerMessage
{
	Welcome = 4567,
	Unwelcome,
	Replication,
	Ping
};
