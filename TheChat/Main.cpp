// Copyleft.

#include <algorithm>
#include <cassert>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")


using namespace std;

namespace
{
	bool InitializeWinSock()
	{
		WSADATA wsaData;
		auto result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		
		if (result != 0)
		{
			cout << "Initialize Winsock failed. Error = " << result << endl;
			
			return false;
		}

		return true;
	}

	void DeinitWinSock()
	{
		WSACleanup();
	}

	bool IsSocketAlive(SOCKET socket)
	{
		if (socket == INVALID_SOCKET)
			return false;

		int optVal;
		int optLen = sizeof(int);

		if (getsockopt(socket, SOL_SOCKET, SO_ERROR, (char*)&optVal, &optLen) == SOCKET_ERROR)
			return false;

		bool isAlive = optVal == 0;
		
		return isAlive;
	}
}

class ChatServer final
{
public:
	static constexpr int BUFFER_SIZE = 4096;
	static constexpr int BUFFER_MAX = BUFFER_SIZE - 1;
	static constexpr int DEFAULT_PORT = 8089;

private:
	string port;
	thread chatThread;
	SOCKET listenSocket;

	atomic<bool> isRunning;
	mutex connectionsLock;
	vector<SOCKET> connections;

public:
	ChatServer(const char* port)
		: port(port)
		, listenSocket(INVALID_SOCKET)
	{
	}
	
	~ChatServer()
	{
		isRunning = false;

		if (chatThread.joinable())
		{
			chatThread.join();
		}

		Release();
	}

	void Run()
	{
		isRunning = true;
	
		StartChatThread();

		listenSocket = INVALID_SOCKET;
		
		struct addrinfo* addrInfo = nullptr;
		struct addrinfo hints;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		int result = getaddrinfo(NULL, port.c_str(), &hints, &addrInfo);
		if (result != 0)
		{
			cerr << "[TheChatServer] getaddrinfo failed. error = " << result << endl;
			
			return;
		}

		listenSocket = ::socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
		if (listenSocket == INVALID_SOCKET)
		{
			cerr << "[TheChatServer] socket failed. error = " << result << endl;

			freeaddrinfo(addrInfo);
			return;
		}

		result = bind(listenSocket, addrInfo->ai_addr, (int)addrInfo->ai_addrlen);
		if (result == SOCKET_ERROR)
		{
			cerr << "[TheChatServer] bind failed. error = " << WSAGetLastError() << endl;

			freeaddrinfo(addrInfo);
			closesocket(listenSocket);
			return;
		}

		freeaddrinfo(addrInfo);

		result = listen(listenSocket, SOMAXCONN);
		if (result == SOCKET_ERROR)
		{
			cerr << "[TheChatServer] listen failed. error = " << WSAGetLastError() << endl;

			closesocket(listenSocket);
			return;
		}
		
		cout << "[TheChatServer] Listen port = " << port << endl;

		isRunning = true;
		while (isRunning)
		{
			auto clientSocket = accept(listenSocket, NULL, NULL);
			if (clientSocket == INVALID_SOCKET)
			{
				cerr << "[TheChatServer] accept failed, error = " << WSAGetLastError() << endl;
				continue;
			}

			u_long nonBlockingMode = 1;
			if (ioctlsocket(clientSocket, FIONBIO, &nonBlockingMode) != 0)
			{
				cerr << "[TheChatServer] ioctlsocket failed, error = " << WSAGetLastError() << endl;
				
				shutdown(clientSocket, SD_SEND);
				closesocket(clientSocket);
				clientSocket = INVALID_SOCKET;
				continue;
			}

			{
				lock_guard<mutex> lock(connectionsLock);

				connections.push_back(clientSocket);
				clientSocket = INVALID_SOCKET;

				auto func = [](const auto& item) -> bool
				{
					bool isAlive = IsSocketAlive(item);
					if (!isAlive)
					{
						cout << "[TheChatServer] client connection closed." << endl;
						return true;
					}

					return false;
				};

				for (auto iter = connections.begin(); iter != connections.end(); ++iter)
				{
					if (!func(*iter))
						continue;

					iter = connections.erase(iter);
				}
			}

		}

		Release();
	}

private:
	void StartChatThread()
	{
		auto Func = [this]()
		{
			fd_set readSet;
			fd_set writeSet;
			timeval tval{ 0, 0 };
			
			char buffer[BUFFER_SIZE];
			vector<string> textBuffer;
			vector<SOCKET> quitList;

			string shutdownMsg("shutdown");
			string quitMsg("quit");

			while (isRunning)
			{				
				lock_guard<mutex> lock(connectionsLock);
				const auto numConnections = connections.size();
				if (numConnections < 1)
					continue;

				FD_ZERO(&readSet);
				FD_ZERO(&writeSet);

				for (auto& connection : connections)
				{
					FD_SET(connection, &readSet);
					FD_SET(connection, &writeSet);
				}

				int count = select(0, &readSet, &writeSet, nullptr, &tval);
				if (count <= 0)
					continue;

				bool isShutdown = false;
				quitList.clear();
				
				for (auto& connection : connections)
				{
					if (!FD_ISSET(connection, &readSet))
						continue;

					int recvBufferLen = BUFFER_MAX;
					int recvBytes = recv(connection, buffer, recvBufferLen, 0);
					if (recvBytes < 1)
						continue;

					recvBytes = max(recvBytes, 0);
					recvBytes = min(recvBytes, BUFFER_MAX);
					buffer[recvBytes] = '\0';
					
					std::string rcvText(buffer);
					if (!isShutdown && rcvText == shutdownMsg)
					{
						isShutdown = true;
					}

					if (rcvText == quitMsg)
					{
						quitList.push_back(connection);
					}

					textBuffer.emplace_back(move(rcvText));
				}

				for (auto& quitClient : quitList)
				{
					connections.erase(std::find(connections.begin(), connections.end(), quitClient));
					shutdown(quitClient, SD_BOTH);
					closesocket(quitClient);
				}

				for (auto& connection : connections)
				{
					if (!FD_ISSET(connection, &writeSet))
						continue;

					for (const auto& text : textBuffer)
					{
						send(connection, &text[0], static_cast<int>(text.size() + 1), 0);
					}
				}


				textBuffer.clear();

				if (isShutdown)
				{
					isRunning = false;
				}
			}

			Release();
		};

		chatThread = thread(Func);
	}

	void Release ()
	{
		isRunning = false;

		{
			lock_guard<mutex> lock(connectionsLock);
			for (auto& connection : connections)
			{
				shutdown(connection, SD_SEND);
				closesocket(connection);
			}

			connections.clear();
		}

		if (listenSocket == INVALID_SOCKET)
			return;

		auto result = shutdown(listenSocket, SD_SEND);
		if (result == SOCKET_ERROR)
		{
			cerr << "[TheChatServer] shutdown failed." << endl;
		}

		closesocket(listenSocket);
		listenSocket = INVALID_SOCKET;
	}
};

class ChatClient final
{
private:
	string address;
	string port;
	SOCKET socket;

public:
	ChatClient(const char* address, const char* port)
		: address(address)
		, port(port)
		, socket(INVALID_SOCKET)
	{
		cout << "[TheChatClient] Trying to connect to " << address << ":" << port << endl;
	}

	~ChatClient()
	{
		Release();
	}

	void Run()
	{
		Release();

		struct addrinfo* addressInfo = nullptr;
		struct addrinfo* ptr = nullptr;
		struct addrinfo hints;
		
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		// Resolve the server address and port
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

		fd_set readSet;
		fd_set writeSet;
		timeval tval{ 0, 0 };

		vector<string> inputs;
		
		char buffer[ChatServer::BUFFER_SIZE];
		string shutdownMsg("shutdown");
		string quitMsg("quit");

		mutex inputsMutex;

		atomic<bool> isRunning = true;
		auto inputFunc = [this, &inputsMutex, &isRunning, &inputs]()
		{
			while (isRunning)
			{
				string sendMsg;
				std::getline(std::cin, sendMsg);
				sendMsg.substr(0, ChatServer::BUFFER_MAX);

				{
					lock_guard<mutex> lock(inputsMutex);
					inputs.emplace_back(move(sendMsg));
				}
			}
		};

		thread rcvThread(inputFunc);

		while (isRunning)
		{
			if (socket == INVALID_SOCKET)
				break;

			if (!IsSocketAlive(socket))
				break;

			FD_ZERO(&readSet);
			FD_ZERO(&writeSet);
			FD_SET(socket, &readSet);
			FD_SET(socket, &writeSet);

			int count = select(0, &readSet, &writeSet, nullptr, &tval);
			if (count <= 0)
				continue;

			if (FD_ISSET(socket, &readSet))
			{
				int recvBufferLen = ChatServer::BUFFER_MAX;
				int recvBytes = recv(socket, buffer, recvBufferLen, 0);
				if (recvBytes > 0)
				{
					recvBufferLen = min(recvBufferLen, ChatServer::BUFFER_MAX);
					buffer[recvBufferLen] = '\0';
					buffer[ChatServer::BUFFER_MAX] = '\0';

					string rcvText(buffer);
					cout << "[TheChat] rcv: " << rcvText << endl;

					if (rcvText == shutdownMsg)
					{
						isRunning = false;
					}
				}

				continue;
			}

			if (FD_ISSET(socket, &writeSet))
			{
				
				buffer[ChatServer::BUFFER_MAX] = '\0';
				string sendMsg(buffer);
				
				lock_guard<mutex> lock(inputsMutex);
				for (auto& msg : inputs)
				{
					if (isRunning && msg == quitMsg)
					{
						isRunning = false;
					}

					send(socket, msg.c_str(), static_cast<int>(msg.size() + 1), 0);
				}

				inputs.clear();
			}
		}

		rcvThread.detach();
		Release();
	}

private:
	void Release()
	{
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
};

int main(int argc, const char* argv[])
{
	InitializeWinSock();

	if (argc < 2)
	{
		ChatServer server("8089");
		server.Run();
	}
	else if (argc < 3)
	{
		ChatServer server(argv[1]);
		server.Run();
	}
	else
	{
		ChatClient client(argv[1], argv[2]);
		client.Run();
	}

	DeinitWinSock();

	return 0;
}