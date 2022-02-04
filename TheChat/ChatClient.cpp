
#include "ChatClient.h"

#include <algorithm>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <ws2tcpip.h>

#include "ChatConnection.h"
#include "ChatConstant.h"
#include "ChatPacket.h"


using namespace std;

namespace
{
	static constexpr int BUFFER_SIZE = ChatConstant::LARGE_PACKET_SIZE;
	static constexpr int BUFFER_LAST_INDEX = BUFFER_SIZE - 1;
	static constexpr int MAX_MSG_LENGTH = ChatPacket::PAYLOAD_SIZE - 1;
	static_assert(MAX_MSG_LENGTH < BUFFER_SIZE, "MAX_MSG_LENGTH is bigger than or equal to BUFFER_SIZE.");
}

ChatClient::ChatClient(const char* address, const char* port)
	: address(address)
	, port(port)
	, socket(INVALID_SOCKET)
{
	cout << "[TheChat] Trying to connect to " << address << ":" << port << endl;
}

ChatClient::~ChatClient()
{
	Release();
}

void ChatClient::Run()
{
	Release();

	struct addrinfo* addressInfo = nullptr;
	struct addrinfo* ptr = nullptr;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	auto result = getaddrinfo(address.c_str(), port.c_str(), &hints, &addressInfo);
	if (result != 0)
	{
		cerr << "[TheChat] getaddrinfo failed. error = " << result << endl;
		return;
	}

	for (ptr = addressInfo; ptr != NULL; ptr = ptr->ai_next)
	{
		socket = ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (socket == INVALID_SOCKET)
		{
			cerr << "[TheChat] socket failed. errpr = " << WSAGetLastError() << endl;

			return;
		}

		result = connect(socket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (result == SOCKET_ERROR)
		{
			closesocket(socket);
			socket = INVALID_SOCKET;
			continue;
		}

		break;
	}

	freeaddrinfo(addressInfo);

	u_long nonBlockingMode = 1;
	if (ioctlsocket(socket, FIONBIO, &nonBlockingMode) != 0)
	{
		cerr << "[TheChat] ioctlsocket failed, error = " << WSAGetLastError() << endl;

		shutdown(socket, SD_SEND);
		closesocket(socket);
		socket = INVALID_SOCKET;

		return;
	}

	isRunning = true;
	connection = ChatConnection(socket);

	StartHeartBeatThread();
	StartStdInputThread();

	fd_set readSet;
	fd_set writeSet;
	timeval tval{ 0, 0 };
	static const string quitMsg("quit");

	while (isRunning)
	{
		if (!connection.IsAlive())
		{
			cout << "[TheChat] disconnected from the server." << endl;
			break;
		}
			
		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		FD_SET(socket, &readSet);
		FD_SET(socket, &writeSet);

		int count = select(0, &readSet, &writeSet, nullptr, &tval);
		if (count <= 0)
			continue;

		if (FD_ISSET(socket, &readSet))
		{
			vector<ChatPacket> packets;

			{
				lock_guard<mutex> lock(connectionMutex);

				connection.Receive();
				packets = connection.ExtractReceived();
			}
			
			for (auto& packet : packets)
			{
				if (packet.header.tableId == ChatPacket::MSG_TABLE_ID)
				{
					cout << "rcv: " << (const char*)(packet.payload) << endl;
					continue;
				}

				// TODO
			}

			continue;
		}

		if (FD_ISSET(socket, &writeSet))
		{
			{
				lock_guard<mutex> lock(stdInputBufferMutex);
				lock_guard<mutex> connLock(connectionMutex);

				for (auto& msg : stdInputBuffer)
				{
					if (isRunning && msg == quitMsg)
					{
						isRunning = false;
					}

					ChatPacket msgPacket;
					msgPacket.header.tableId = ChatPacket::MSG_TABLE_ID;

					int strLen = std::min<int>(static_cast<int>(msg.size()), MAX_MSG_LENGTH);
					;
					for (int i = 0; i < strLen; ++i)
					{
						msgPacket.payload[i] = msg.at(i);
					}

					msgPacket.payload[strLen] = '\0';
					connection.RequestSend(msgPacket);
				}

				stdInputBuffer.clear();
			}

			{
				lock_guard<mutex> connLock(connectionMutex);
				connection.FlushSendRequests();
			}
		}
	}

	isRunning = false;

	heartBeatThread.join();
	stdInputThread.detach();

	Release();
}

void ChatClient::StartHeartBeatThread()
{
	auto func = [this]()
	{
		while (isRunning)
		{
			{
				lock_guard<mutex> connLock(connectionMutex);
				if (!connection.IsAlive())
				{
					isRunning = false;
					return;
				}

				connection.SendHeartBeat();
			}
			
			cout << "SendHeart Beat" << endl;
			std::this_thread::sleep_for(chrono::milliseconds(ChatConstant::HEART_BEAT_PERIOD));
		}
	};

	heartBeatThread = thread(func);
}

void ChatClient::StartStdInputThread()
{
	auto inputFunc = [this]()
	{
		while (isRunning)
		{
			string sendMsg;
			std::getline(std::cin, sendMsg);
			sendMsg = sendMsg.substr(0, MAX_MSG_LENGTH);

			{
				lock_guard<mutex> lock(stdInputBufferMutex);
				stdInputBuffer.emplace_back(move(sendMsg));
			}
		}
	};

	stdInputThread = thread(inputFunc);
}

void ChatClient::Release()
{
	isRunning = false;

	if (socket == INVALID_SOCKET)
		return;

	auto result = shutdown(socket, SD_SEND);
	if (result == SOCKET_ERROR)
	{
		cerr << "[TheChatClient] shutdown failed." << endl;
	}

	closesocket(socket);
	socket = INVALID_SOCKET;
}
