#pragma once

#include <string>

#include "ChatConstant.h"
#include "ChatPacket.h"
#include "ChatTableID.h"


class GreetingsPacket final
{
public:
	static constexpr EChatTableID GetTableID() { return EChatTableID::GREETINGS_TABLE; }

public:
	union
	{
		ChatPacket packet;
		struct
		{
			ChatPacket::Header header;
			char senderId[ChatConstant::ID_LENGTH + 1];
		};
	};

	static_assert((sizeof(header) + sizeof(senderId)) <= sizeof(ChatPacket), "GreetingsPacket size overflow.");

	GreetingsPacket(const std::string& id);
	~GreetingsPacket() = default;

	inline const char* GetSenderID() const { return static_cast<const char*>(senderId); }
};