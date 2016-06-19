#include "stdafx.h"
#include "Server.h"




Server::Server(int defaultPort = 8765)
{
	// create WSADATA object
	WSADATA wsaData;

	// our sockets for the server
	ListenSocket = INVALID_SOCKET;

	// address info for the server to listen to
	struct addrinfo *result = NULL;
	struct addrinfo hints;

	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		exit(1);
	}

	// set address information
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;    // TCP connection!!!
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	char tmpBuffer[16]; 
	_itoa_s(defaultPort, tmpBuffer, 16, 10);
	PCSTR portNumAsStr = tmpBuffer;
	iResult = getaddrinfo(NULL, portNumAsStr, &hints, &result);

	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		exit(1);
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		exit(1);
	}

	// Set the mode of the socket to be nonblocking
	u_long iMode = 1;
	iResult = ioctlsocket(ListenSocket, FIONBIO, &iMode);

	if (iResult == SOCKET_ERROR) {
		printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		exit(1);
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);

	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		exit(1);
	}

	// no longer need address information
	freeaddrinfo(result);

	// start listening for new clients attempting to connect
	iResult = listen(ListenSocket, SOMAXCONN);

	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		exit(1);
	}
}


unsigned int Server::multiIO()
{
	struct fd_set readfds,
				  writefds;
	int iResult,
		i;

	while (1) 
	{
		// The fd sets should be zeroed out before using them to prevent errors.
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		// Set the server socket
		FD_SET(ListenSocket, &readfds);
		for (auto it = sessions.cbegin(); it != sessions.cend(); ++it)
		{
			FD_SET(sessions[it->first]->socketHandle, &readfds);
		}

		iResult = select(0, &readfds, &writefds, NULL, NULL);
		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "select failed %d\n", WSAGetLastError());
			exit(1);
		}

		// Check for accepted client connection
		if (FD_ISSET(ListenSocket, &readfds)) 
		{
			// Create the socket type we need to get client information such as IP address:
			struct sockaddr_in client_info = { 0 };
			int sizeOfSock = sizeof(client_info);

			// Accept the connection
			SOCKET ClientSocket = accept(ListenSocket, (sockaddr*)&client_info /* this method takes sockaddr instead of sockaddr_in structure, so we cast it */, &sizeOfSock);

			if (ClientSocket != INVALID_SOCKET)
			{
				//Disable nagle algorithim on the socket. (Nagle algorithm reduces netowrk traffict by buffering small packets to combine them and then flush them all at once)
				char value = 1;
				setsockopt(ClientSocket, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));

				// Create a new Connection object to represent this connection/session and add it to our table of connections:
				Connection* ptr = new Connection(IDCounter, ClientSocket, client_info);
				sessions.insert(std::pair<unsigned int, Connection*>(IDCounter, ptr));
				printf("\nNew connection from IP %s. (ID %d)\n", ptr->IPAddress, IDCounter);
				IDCounter++;
			}
		}
		else
		{
			for (auto it = sessions.cbegin(); it != sessions.cend(); ++it)
			{
				if (FD_ISSET(sessions[it->first]->socketHandle, &readfds))
				{
					Connection* con = sessions[it->first];
					bool deleteConnection = false;

					if (con->ReadNetworkMessageSize() > 0)
					{
						if (con->NetworkMessageIsComplete('\r'))
						{
							char* message = con->GetNetworkMessage();
							printf("\nNew message from client %d: '%s'", con->connectionID, message);
							if (strcmp(message, "exit") == 0)
								deleteConnection = true;

							int msgLength = -1; while (message[++msgLength] != '\0' && msgLength < NETWORK_BUFFER_SIZE);
							char* relayMessage = new char[NETWORK_BUFFER_SIZE];
							int relayMessageSize = sprintf_s(relayMessage, NETWORK_BUFFER_SIZE, "User %d (%s): %s\n\r\n\r", con->connectionID, con->IPAddress, message);
							for (auto subIt = sessions.cbegin(); subIt != sessions.cend(); ++subIt)
								sessions[subIt->first]->SendNetworkMessage(relayMessage, relayMessageSize);
							//con->SendNetworkMessage(relayMessage, relayMessageSize);
						}
					}
					else
						deleteConnection = true;

					if (deleteConnection)
					{
						printf("\nClient %d: requested disconnect.", con->connectionID);
						con->Shutdown();
						sessions.erase(it);
					}

					break;
				}
			}
		}

	}
}




// When destroying this class, close all connections:
Server::~Server()
{
	// Close out any existing connections...
	printf("\nClosing connections...");
	for (std::map<unsigned int, Connection*>::iterator iter = sessions.begin(); iter != sessions.end(); ++iter)
		sessions[iter->first]->Shutdown();
}

