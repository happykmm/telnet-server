#include <winsock2.h>
#include <WS2tcpip.h>
#pragma once

#include <map>
#include "Connection.h"
#pragma comment (lib, "Ws2_32.lib")

class Server
{
private:
	// This counter will increase by one for each connection, giving us our own ID for connections from clients.
	unsigned int IDCounter = 1;

	// This will be the socket handle we use to listen to incoming connections: 
	SOCKET ListenSocket;

public:
	Server(int defaultPort);
	~Server();

	// This data structure will hold all our connections in a table:
	std::map<unsigned int, Connection*> sessions;

	// Listens for ListenSocket and all client sockets
	unsigned int multiIO();
};

