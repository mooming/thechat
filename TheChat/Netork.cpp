
#include "Network.h"

#include <iostream>

#define WIN32_LEAN_AND_MEAN

#include <mstcpip.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")


using namespace std;

bool Network::Initialize()
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

void Network::Deinit()
{
	WSACleanup();
}
