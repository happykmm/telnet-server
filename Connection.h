#include <winsock2.h>
#include <WS2tcpip.h>
#pragma once

#define NETWORK_BUFFER_SIZE 256

class Connection
{
private:
	char* internalBuffer = new char[NETWORK_BUFFER_SIZE];
	int internalBufferIndex = 0;
	int internalBufferEndToken = 1;

public:
	SOCKET socketHandle = 0;
	char* IPAddress = nullptr;
	unsigned int connectionID = 0;

	Connection();
	Connection(unsigned int, SOCKET, sockaddr_in);
	~Connection();

	void Shutdown();
	bool SendNetworkMessage(char * message, int length);

	int ReadNetworkMessageSize();
	bool NetworkMessageIsComplete(char delimiter);
	char * GetNetworkMessage();
};

