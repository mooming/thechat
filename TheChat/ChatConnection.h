#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "ChatConstant.h"
#include "ChatPacket.h"
#include "Network.h"


class ChatConnection final
{
private:
	bool isAlive;
	std::string identifier;
	std::string address;
	Network::TSocket socket;
	Network::TTimeStamp timeStamp;

	std::vector<ChatPacket> receivedPackets;
	std::vector<ChatPacket> packetsToBeSent;

	uint8_t receiveBuffer[ChatConstant::PACKET_SIZE];

public:
	ChatConnection();
	ChatConnection(Network::TSocket socket);
	~ChatConnection();

	inline bool operator == (const ChatConnection& rhs) const { return socket == rhs.socket; }
	inline bool operator != (const ChatConnection& rhs) const { return socket != rhs.socket; }
	void Close();

	bool IsAlive() const;
	void RequestSend(const ChatPacket& packet);
	void Receive();

	std::vector<ChatPacket> ExtractReceived();
	void FlushSendRequests();

	void SendHeartBeat();
	void SetID(const char* id);

	inline auto& GetID() const { return identifier; }
	inline auto& GetAddress() const { return address; }
	inline auto GetSocket() { return socket; }
};