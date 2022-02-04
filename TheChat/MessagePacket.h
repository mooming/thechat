#pragma once

#include <string>

#include "ChatConstant.h"
#include "ChatPacket.h"
#include "ChatTableID.h"


class MessagePacket final
{
public:
	static constexpr int MESSAGE_LENGTH = 128;
	static constexpr EChatTableID GetTableID() { return EChatTableID::MESSAGE_TABLE; }

public:
	union
	{
		ChatPacket packet;
		struct
		{
			ChatPacket::Header header;
			char senderId[ChatConstant::ID_LENGTH + 1];
			char message[MESSAGE_LENGTH + 1];
		};
	};

	static_assert((sizeof(header) + sizeof(senderId) + sizeof(message)) <= sizeof(ChatPacket), "Message packet size overflow.");

	MessagePacket();
	~MessagePacket() = default;

	void SetSenderID(const std::string& id);
	int SetMessage(const std::string& text, int offset = 0);

	void Validate();

	inline const char* GetSenderID() const { return static_cast<const char*>(senderId); }
	inline const char* GetMessage() const { return static_cast<const char*>(message); }
};