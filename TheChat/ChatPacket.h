#pragma once

#include <cassert>
#include <cstdint>

#include "ChatConstant.h"
#include "ChatTableID.h"


struct ChatPacket final
{
public:
	enum class EPacketType : uint8_t
	{
		Normal = 0,
		Request = 1
	};

public:
	struct Header
	{
		EChatTableID tableId = EChatTableID::HEARTBEAT;
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

	template<typename T>
	T& As()
	{
		assert(T::GetTableID() == header.tableId);
		return reinterpret_cast<T&>(*this);
	}

	template<typename T>
	const T& As() const
	{
		assert(T::GetTableID() == header.tableId);
		return reinterpret_cast<const T&>(*this);
	}

	template<typename T>
	static ChatPacket& From(T& packet)
	{
		return reinterpret_cast<ChatPacket&>(packet);
	}

	template<typename T>
	static const ChatPacket& From(const T& packet)
	{
		return reinterpret_cast<const ChatPacket&>(packet);
	}
};
