#include "stdafx.h"
#include "Connection.h"


Connection::Connection() { printf("\nEmpty constructor called\n"); }

Connection::Connection(unsigned int ID, SOCKET socketHdl, sockaddr_in socketAdd)
{
	this->connectionID = ID;
	this->socketHandle = socketHdl;
	this->IPAddress = new char[20];
	inet_ntop(AF_INET, &socketAdd.sin_addr, this->IPAddress, 20);

	this->internalBufferEndToken = -1;
	memset(this->internalBuffer, 0, NETWORK_BUFFER_SIZE);
	this->internalBufferIndex = 0;
}

void Connection::Shutdown()
{
	if (this->connectionID == 0) return;
	printf("\nShutting down connection with client %d\n", this->connectionID);
	int iResult = shutdown(this->socketHandle, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("\nShutting down connection %d failed with error: %d\n", this->connectionID, WSAGetLastError());
	}
	// cleanup
	closesocket(this->socketHandle);
}

bool Connection::SendNetworkMessage(char* message, int length)
{
	if (length > NETWORK_BUFFER_SIZE) length = NETWORK_BUFFER_SIZE;
	int iSendResult = send(this->socketHandle, message, length, 0);
	return !(iSendResult == SOCKET_ERROR);
}

int Connection::ReadNetworkMessageSize()
{
	char *tempBuffer = new char[NETWORK_BUFFER_SIZE];
	int receivedBytes = recv(this->socketHandle, tempBuffer, NETWORK_BUFFER_SIZE, 0);
	if (receivedBytes > 0)
	{
		int sizeToCopy = NETWORK_BUFFER_SIZE - (this->internalBufferIndex + receivedBytes - 1);
		if (sizeToCopy > 0)
		{
			strncpy_s(this->internalBuffer + this->internalBufferIndex, (NETWORK_BUFFER_SIZE - this->internalBufferIndex),
				tempBuffer, receivedBytes);
			if ((this->internalBufferIndex + receivedBytes) == (NETWORK_BUFFER_SIZE - 1))
				this->internalBuffer[(NETWORK_BUFFER_SIZE - 1)] = '\0';
			this->internalBufferIndex += receivedBytes;
			delete[] tempBuffer;
			return sizeToCopy;
		}
		else if (sizeToCopy < 0)
		{
			printf("\nReceived additional data from client %d but buffer was full. (%d overflow)\n", this->connectionID, abs(sizeToCopy));
		}
	}
	delete[] tempBuffer;
	return receivedBytes;
}

bool Connection::NetworkMessageIsComplete(char delimiter)
{
	if (this->internalBufferEndToken > -1) return true;
	if (this->internalBufferIndex == (NETWORK_BUFFER_SIZE - 1)) return true;
	bool messageComplete = false;
	int x = 0; 
	while (x < NETWORK_BUFFER_SIZE && !messageComplete)
	{
		if (this->internalBuffer[x++] == delimiter)
		{
			this->internalBufferEndToken = x - 1;
			messageComplete = true;
		}
	}
	return messageComplete;
}

char* Connection::GetNetworkMessage()
{
	char* message = new char[NETWORK_BUFFER_SIZE];
	int copyLength = this->internalBufferEndToken;

	if (copyLength == -1)
		copyLength = this->internalBufferIndex;

	strncpy_s(message, copyLength + 1, this->internalBuffer, copyLength);

	if (this->internalBufferEndToken >= 0 || copyLength == (NETWORK_BUFFER_SIZE - 1))
	{
		this->internalBufferEndToken = -1;
		memset(this->internalBuffer, 0, NETWORK_BUFFER_SIZE);
		this->internalBufferIndex = 0;
	}

	return message;
}

Connection::~Connection()
{
	Shutdown();
}
