#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include "ChatConnection.h"
#include "Network.h"


class ChatClient final
{
private:
	std::atomic<bool> isRunning;
	std::string address;
	std::string port;
	Network::TSocket socket;

	ChatConnection connection;
	std::vector<std::string> stdInputBuffer;

	std::mutex connectionMutex;
	std::mutex stdInputBufferMutex;
	std::thread stdInputThread;
	std::thread heartBeatThread;

public:
	ChatClient(const char* address, const char* port);
	~ChatClient();

	void Run();

private:
	void StartHeartBeatThread();
	void StartStdInputThread();

	void Release();
};