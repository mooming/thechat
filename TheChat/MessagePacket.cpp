#include "MessagePacket.h"

#include <algorithm>


using namespace std;

MessagePacket::MessagePacket()
	: header()
	, senderId{'\0', }
	, message{'\0', }
{
	header.tableId = GetTableID();
}

void MessagePacket::SetSenderID(const string& id)
{
	const int length = std::min<int>(static_cast<int>(id.size()), ChatConstant::ID_LENGTH);
	
	int i = 0;
	for (; i < length; ++i)
	{
		senderId[i] = id.at(i);
	}

	senderId[i] = '\0';
}

int MessagePacket::SetMessage(const string& text, int offset)
{
	int i = 0;
	int length = static_cast<int>(text.size()) - offset;
	length = std::min(length, MESSAGE_LENGTH);

	for (; i < length; ++i)
	{
		message[i] = text.at(i + offset);
	}

	message[i] = '\0';

	return i + offset;
}

void MessagePacket::Validate()
{
	senderId[sizeof(senderId) - 1] = '\0';
	message[sizeof(message) - 1] = '\0';
}

