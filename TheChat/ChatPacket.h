#pragma once

#include <cstdint>

#include "ChatConstant.h"


struct ChatPacket final
{
public:
	static constexpr int MSG_TABLE_ID = 0;

	enum class EPacketType : uint8_t
	{
		Normal = 0,
		Large = 1
	};

public:
	struct Header
	{
		uint16_t tableId = 0;
		uint8_t tableVersion = 0;
		EPacketType packetType = EPacketType::Normal;
		uint8_t index = 0;
		uint8_t maxIndex = 0;
		uint16_t payloadLength = 0;
	};

	static constexpr int PAYLOAD_SIZE = ChatConstant::PACKET_SIZE - sizeof(Header);
	union
	{
		uint8_t data[ChatConstant::PACKET_SIZE];
		struct
		{
			Header header;
			uint8_t payload[PAYLOAD_SIZE];
		};
	};

	ChatPacket();
	~ChatPacket() = default;
};