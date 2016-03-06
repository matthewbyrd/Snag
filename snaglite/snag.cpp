#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <iostream>

// Make the C socket stuff more pleasant
#define IPV4 AF_INET
#define TCP SOCK_STREAM
const uint8_t defaultProtocol = 0;
typedef int Socket;
typedef struct sockaddr_in SocketAddress;
typedef struct hostent Host;  

int connectToServer();

int main(int argc, char* argv[])
{
  if (argc < 2) 
  {
    std::cout << "USAGE: " << argv[0] << " [-l / -u][snaggable]" << std::endl;
		std::cout << "Examples:" << std::endl;
		std::cout << argv[0] << " -l            <-- list the servers" << std::endl;
		std::cout << argv[0] << " myserver      <-- attempt to snag myserver" << std::endl;
		std::cout << argv[0] << " -u myserver   <-- attempt to unsnag myserver" << std::endl;
		std::cout << argv[0] << " -a myserver   <-- attempt to add myserver" << std::endl;
		std::cout << argv[0] << " -d myserver   <-- attempt to delete myserver" << std::endl;
		
    return 1;
  }

	Socket clientSocket = connectToServer();
	if (clientSocket == -1)
	{
		return 1;
	}
		
	char clientName[20];
	getlogin_r(clientName, 20);
	
	int writeLen;
	if ( strcmp(argv[1], "-l") == 0 )
	{
		write(clientSocket, "l-", 2);
	}	
	else if ( strcmp(argv[1], "-a") == 0)
	{
		if (argc < 3)
		{
			std::cout << "ERROR: you didn't specify a server to add!" << std::endl;
			return 1;
		}
		
		std::string message = "a-";
		message += argv[2];
		message += "-";
		message += clientName;
		message += "-";
		write(clientSocket, message.c_str(), message.size());
	}
	else if ( strcmp(argv[1], "-d") == 0)
	{
		if (argc < 3)
		{
			std::cout << "ERROR: you didn't specify a server to delete!" << std::endl;
			return 1;
		}
		
		std::string message = "d-";
		message += argv[2];
		message += "-";
		message += clientName;
		message += "-";
		write(clientSocket, message.c_str(), message.size());
	}
	else if ( strcmp(argv[1], "-u") == 0 )
	{
		if (argc < 3)
		{
			std::cout << "ERROR: you didn't specify a server to unsnag!" << std::endl;
			return 1;
		}
		
		std::string message = "u-";
		message += argv[2];
		message += "-";
		message += clientName;
		message += "-";
		write(clientSocket, message.c_str(), message.size());
	}
	else
	{
		std::string message = "s-";
		message += argv[1];
		message += "-";
		message += clientName;
		message += "-";
		write(clientSocket, message.c_str(), message.size());
	}
	
	// get server reply
	std::cout << std::endl;
	char buffer[1024];
	memset(buffer, 0, 1024);
	int readLen = read(clientSocket, buffer, 2048);
	if (readLen < 0)
	{
	  std::cerr << "ERROR reading from socket" << std::endl;
	  return 1;
	}
	std::cout << buffer << std::endl;
	std::cout << std::endl;
	
}


Socket connectToServer()
{
  int portNumber = atoi("6647");
  int clientSocket = socket(IPV4, SOCK_STREAM, defaultProtocol);
  if (clientSocket < 0) 
  {
    std::cerr << "ERROR opening socket" << std::endl;
    return -1;
  }
  
  Host* server = gethostbyname("localhost");
  if (server == NULL)
  {
    std::cerr << "ERROR, no such host " << std::endl;
    return -1;
  }
	
  SocketAddress serverAddress;
  memset((char*)&serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin_family = IPV4;
  memcpy((char *)&serverAddress.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
  serverAddress.sin_port = htons(portNumber);
  if (connect(clientSocket,(const sockaddr*)&serverAddress,sizeof(serverAddress)) < 0)
  {
    std::cerr << "ERROR connecting" << std::endl;
    return -1;
  }
	return clientSocket;
}