#include "GreetingsPacket.h"


GreetingsPacket::GreetingsPacket(const std::string& id)
	: packet()
{
	header.tableId = GetTableID();

	const int length = std::min<int>(static_cast<int>(id.size()), ChatConstant::ID_LENGTH);

	int i = 0;
	for (; i < length; ++i)
	{
		senderId[i] = id.at(i);
	}

	senderId[i] = '\0';
}