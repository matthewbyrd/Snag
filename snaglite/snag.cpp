#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <iostream>
#include <sstream>

// Make the C socket stuff more pleasant
#define IPV4 AF_INET
#define TCP SOCK_STREAM
const uint8_t defaultProtocol = 0;
typedef int Socket;
typedef struct sockaddr_in SocketAddress;
typedef struct hostent Host;  

int connectToServer();
void parseSnagList(char* buffer, size_t len);

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

	if ( strcmp(argv[1], "-l") == 0 )
	{
		write(clientSocket, "l-", 2);
		std::cout << std::endl;
		std::cout << "    MACHINE NAME          SNAGGER          SNAGGED FOR" << std::endl;
		std::cout << "----------------------------------------------------------" << std::endl;
		
		// get server reply
		char buffer[601];
		memset(buffer, 0, 601);
		int readLen = read(clientSocket, buffer, 400);
		if (readLen < 0)
		{
		  std::cerr << "ERROR reading from socket" << std::endl;
		  return 1;
		}
		parseSnagList(buffer, readLen);
		return 0; 
	}	
	else if ( strcmp(argv[1], "-a") == 0)
	{
		if (argc < 3)
		{
			std::cout << "ERROR: you didn't specify a server to add!" << std::endl;
			return 1;
		}
		std::ostringstream oss;
		oss << "a-" << argv[2] << "-" << clientName << "-";
		std::string message = oss.str();
		write(clientSocket, message.c_str(), message.size());
	}
	else if ( strcmp(argv[1], "-d") == 0)
	{
		if (argc < 3)
		{
			std::cout << "ERROR: you didn't specify a server to delete!" << std::endl;
			return 1;
		}
		std::ostringstream oss;
		oss << "d-" << argv[2] << "-" << clientName << "-";
		std::string message = oss.str();
		write(clientSocket, message.c_str(), message.size());
	}
	else if ( strcmp(argv[1], "-u") == 0 )
	{
		if (argc < 3)
		{
			std::cout << "ERROR: you didn't specify a server to unsnag!" << std::endl;
			return 1;
		}
		std::ostringstream oss;
		oss << "u-" << argv[2] << "-" << clientName << "-";
		std::string message = oss.str();
		write(clientSocket, message.c_str(), message.size());
	}
	else
	{
		std::ostringstream oss;
		oss << "s-" << argv[1] << "-" << clientName << "-";
		std::string message = oss.str();
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
  int portNumber = atoi("6641");
  int clientSocket = socket(IPV4, SOCK_STREAM, defaultProtocol);
  if (clientSocket < 0) 
  {
    std::cerr << "ERROR opening socket" << std::endl;
    return -1;
  }
  
  Host* server = gethostbyname("Matthews-Air.home");
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

void parseSnagList(char* buffer, size_t len)
{
	std::ostringstream oss;
	for (size_t i = 0; i < len;)
	{
		oss << "    ";
	  ++i;
		// extract snaggable name
		int nameLength = 0;
		while (buffer[i] != '#')
		{
			oss << buffer[i++];
			++nameLength;
		}
		int spaces = 22 - nameLength;
		for (; spaces; --spaces) oss << " ";
		
		++i;
		//check if snagged
		if (!(buffer[i] == '0'))
		{
			// extract snagger info
			// snagger name
			++i;
			++i;
			nameLength = 0;
			while(buffer[i] != '#')
			{
				oss << buffer[i++];
				++nameLength;
			}
			int spaces = 17 - nameLength;
			for (; spaces; --spaces) oss << " ";
			
			// snagged time
			++i;
			while(buffer[i] != '#')
			{
				oss << buffer[i++];
			}
			++i;
		}
		else
		{
			++i;++i;
		}
		oss << "\n";
	}
	std::string output = oss.str();
	std::cout << output << std::endl;
}
