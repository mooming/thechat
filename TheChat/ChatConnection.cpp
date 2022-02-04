#include "ChatConnection.h"

#include <cassert>
#include <iostream>
#include <memory>


using namespace std;

ChatConnection::ChatConnection()
	: isAlive(false)
	, socket(INVALID_SOCKET)
	, timeStamp(std::chrono::steady_clock::now())
	, receiveBuffer{0, }
{
}

ChatConnection::ChatConnection(Network::TSocket socket)
	: isAlive(true)
	, socket(socket)
	, timeStamp(std::chrono::steady_clock::now())
	, receiveBuffer{0, }
{
}

ChatConnection::~ChatConnection()
{
	Close();
}

void ChatConnection::Close()
{
	isAlive = false;
	
	if (socket != INVALID_SOCKET)
		return;

	shutdown(socket, SD_BOTH);
	closesocket(socket);
}

bool ChatConnection::IsAlive() const
{
	if (!isAlive)
		return false;

	const auto currentTime = chrono::steady_clock::now();
	const auto deltaTime = chrono::duration_cast<chrono::seconds>(currentTime - timeStamp);
	const auto deltaSeconds = deltaTime.count();

	return deltaSeconds < ChatConstant::CONNECTION_TIMEOUT;
}

void ChatConnection::RequestSend(const ChatPacket& packet)
{
	packetsToBeSent.emplace_back(packet);
}

void ChatConnection::Receive()
{
	constexpr int MAX_SIZE = ChatConstant::LARGE_PACKET_SIZE;

	int recvBytes = recv(socket, (char*)receiveBuffer, MAX_SIZE, 0);
	if (recvBytes < 1)
	{
		cout << "[ChatConnection] Broken pipe. Client: " << identifier << "@" << address << endl;
		Close();
		return;
	}

	timeStamp = chrono::steady_clock::now();

	if (recvBytes < 2)
	{
		cout << "[ChatConnection] Heart beat received from " << identifier << "@" << address << endl;
		return;
	}

	assert(recvBytes <= MAX_SIZE);
	recvBytes = min(recvBytes, MAX_SIZE);

	auto& header = reinterpret_cast<ChatPacket::Header&>(receiveBuffer[0]);
	switch (header.packetType)
	{
	case ChatPacket::EPacketType::Normal:
		if (recvBytes <= ChatConstant::PACKET_SIZE)
		{
			assert(recvBytes <= ChatConstant::PACKET_SIZE);
			auto& packet = reinterpret_cast<ChatPacket&>(header);
			receivedPackets.emplace_back(packet);
		}
		else
		{
			cerr << "[ChatConnection][Error] Over sized packet is received. It shall be dropped." << endl;
		}
		break;

	case ChatPacket::EPacketType::Large:
		cerr << "[ChatConnection][Error] Large packet is not implemented yet. It shall be dropped." << endl;
		break;

	default:
		cerr << "[ChatConnection][Error] Not handled type: " << static_cast<int>(header.packetType) << endl;
		break;
	}
}

std::vector<ChatPacket> ChatConnection::ExtractReceived()
{
	vector<ChatPacket> extracted;
	swap(extracted, receivedPackets);

	return move(extracted);
}

void ChatConnection::FlushSendRequests()
{
	for (const auto& packet : packetsToBeSent)
	{
		const char* data = reinterpret_cast<const char*>(&packet);
		send(socket, data, sizeof(packet), 0);
	}

	packetsToBeSent.clear();
}

void ChatConnection::SendHeartBeat()
{
	char ch = '\0';
	send(socket, &ch, sizeof(ch), 0);
}

