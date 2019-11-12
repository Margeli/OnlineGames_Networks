#pragma once

enum class ClientMessage
{
	Hello = 8758,
	Input,
	Ping
};

enum class ServerMessage
{
	Welcome = 4567,
	Unwelcome,
	Replication,
	Ping
};
