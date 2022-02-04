#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ChatConnection.h"
#include "ChatPacket.h"
#include "Network.h"


class ChatServer final
{
private:
	std::string port;
	std::thread chatThread;
	Network::TSocket listenSocket;

	std::atomic<bool> isRunning;
	std::mutex connectionsLock;
	std::vector<ChatConnection> connections;

	using TProc = std::function<void(ChatConnection& connection, ChatPacket& packet)>;
	std::map<EChatTableID, TProc> procMap;

public:
	ChatServer(const char* port);
	~ChatServer();

	void Run();

private:
	void Listen();
	void StartChatThread();
	void Release();

	void BuildTableProcessor();
	void ProcessTable(ChatConnection& connection, ChatPacket& packet);
};