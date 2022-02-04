// Copyleft.

#include "ChatClient.h"
#include "ChatServer.h"
#include "Network.h"

#include <iostream>


int main(int argc, const char* argv[])
{
	Network::Initialize();

	using namespace std;

	if (argc < 2)
	{
		cout << "Usage: " << endl;
		cout << "Server: > " << argv[0] << endl;
		cout << "Server: > " << argv[0] << " <port>" << endl;
		cout << "Clinet: > " << argv[0] << "<address> <port> <id>" << endl << endl;

		cout << "Selected Mode: Server" << endl;
		ChatServer server("8089");
		server.Run();
	}
	else if (argc < 3)
	{
		cout << "Selected Mode: Server" << endl;

		ChatServer server(argv[1]);
		server.Run();
	}
	else if (argc < 4)
	{
		cout << "Selected Mode: Client" << endl;

		ChatClient client(argv[1], argv[2], "Unknown");
		client.Run();
	}
	else
	{
		cout << "Selected Mode: Client" << endl;

		ChatClient client(argv[1], argv[2], argv[3]);
		client.Run();
	}

	Network::Deinit();

	return 0;
}