#pragma once

#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ChatConnection.h"
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

public:
	ChatServer(const char* port);
	~ChatServer();

	void Run();

private:
	void Listen();
	void StartChatThread();
	void Release();
};