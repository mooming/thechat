
#include "ChatServer.h"

#include <iostream>

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <ws2tcpip.h>

#include "ChatConstant.h"


using namespace std;

ChatServer::ChatServer(const char* port)
	: port(port)
	, listenSocket(INVALID_SOCKET)
{
}

ChatServer::~ChatServer()
{
	isRunning = false;

	if (chatThread.joinable())
	{
		chatThread.join();
	}

	Release();
}

void ChatServer::Run()
{
	isRunning = true;

	StartChatThread();
	Listen();

	Release();
}

void ChatServer::Listen()
{
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

			connections.emplace_back(clientSocket);
			clientSocket = INVALID_SOCKET;

			auto& connection = connections.back();
			cout << "[TheChatServer] connection established with " << connection.GetID()
				<< '@' << connection.GetAddress() << endl;

			auto func = [](const auto& connection) -> bool
			{
				bool isAlive = connection.IsAlive();
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
}

void ChatServer::StartChatThread()
{
	auto Func = [this]()
	{
		fd_set readSet;
		fd_set writeSet;
		timeval tval{ 0, 0 };

		vector<ChatPacket> sendBuffer;
		vector<Network::TSocket> quitList;

		while (isRunning)
		{
			lock_guard<mutex> lock(connectionsLock);
			
			{
				for (auto it = connections.begin(); it != connections.end();)
				{
					auto& connection = *it;
					if (connection.IsAlive())
					{
						++it;
						continue;
					}
					
					cout << "[TheChatServer] connection closed with " << connection.GetID()
						<< '@' << connection.GetAddress() << endl;

					it = connections.erase(it);
				}
			}

			const auto numConnections = connections.size();
			if (numConnections < 1)
				continue;

			FD_ZERO(&readSet);
			FD_ZERO(&writeSet);

			for (auto& connection : connections)
			{
				FD_SET(connection.GetSocket(), &readSet);
				FD_SET(connection.GetSocket(), &writeSet);
			}

			int count = select(0, &readSet, &writeSet, nullptr, &tval);
			if (count <= 0)
				continue;

			for (auto& connection : connections)
			{
				if (!FD_ISSET(connection.GetSocket(), &readSet))
					continue;

				connection.Receive();
				auto received = connection.ExtractReceived();

				if (received.size() <= 0)
				{
					cout << "[TheChatServer] Heart beat from " << connection.GetID() << '@' << connection.GetAddress() << endl;
				}
				else
				{
					for (auto& connection : connections)
					{
						for (auto& packet : received)
						{
							if (packet.header.tableId == ChatPacket::MSG_TABLE_ID)
							{
								for (auto& peer : connections)
								{
									if (connection == peer)
										continue;

									peer.RequestSend(packet);
								}
							}
						}
					}
				}		
			}

			for (auto& connection : connections)
			{
				if (!FD_ISSET(connection.GetSocket(), &writeSet))
					continue;

				connection.FlushSendRequests();

			}
		}

		Release();
	};

	chatThread = thread(Func);
}

void ChatServer::Release()
{
	isRunning = false;

	{
		lock_guard<mutex> lock(connectionsLock);
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
