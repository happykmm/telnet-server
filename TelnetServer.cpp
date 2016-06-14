#include "stdafx.h"
#include "Server.h"
#include <conio.h>



int main() 
{
	int serverPort = 8765;
	
	Server* serverHandle = new Server(serverPort);
	printf("Server setup and listening on port %d. \n", serverPort);

	serverHandle->multiIO();

}

