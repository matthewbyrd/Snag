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

typedef uint8_t u8;

// Make the C socket stuff more pleasant
#define IPV4 AF_INET
#define TCP SOCK_STREAM
const uint8_t defaultProtocol = 0;
typedef int Socket;
typedef struct sockaddr_in SocketAddress;
typedef struct hostent Host;  

int connectToServer();
void parseSnagList(char* buffer, size_t len);
void helpCommand();
void listCommand(Socket clientSocket);
void snagCommand(Socket clientSocket, std::string& target, std::string& clientName);
void unsnagCommand(Socket clientSocket, std::string& target, std::string& clientName);
void addCommand(Socket clientSocket, std::string& target, std::string& clientName);
void delCommand(Socket clientSocket, std::string& target, std::string& clientName);

enum commands
{
	  snag   = 0
	, unsnag = 1
	, list   = 2
	, add    = 3
	, del    = 4
	, help   = 5
};

int main(int argc, char* argv[])
{
	// verify input
	u8 command = 0;
	std::string target = "";
	
	if (argc == 1)
	{
		command = list;
	}
	else if (argc == 2)
	{
		if (0 == strcmp(argv[1], "-l"))
		{
			command = list;
		}
		else if (0 == strcmp(argv[1], "-h"))
		{
			command = help;
		}
		else if (0 == strcmp(argv[1], "-u") ||
						 0 == strcmp(argv[1], "-a") || 
						 0 == strcmp(argv[1], "-d"))
		{
			std::cerr << "ERROR: you didn't provide a server. ";
			std::cerr << "Type snag -h for help" << std::endl;
			return 1;
		}
		else if (argv[1][0] == '-')
		{
			std::cerr << "ERROR: unknown command '" << argv[1] << "', ";
			std::cerr << "Type snag -h for help" << std::endl;
			return 1;
		}
		else
		{
			command = snag;
			target = argv[1];
		}
	}
	else if (argc > 2)
	{
		if (0 == strcmp(argv[1], "-l"))
		{
			command = list;
			std::cerr << "TIP: list command takes no arguments." << std::endl;
		}
		else if (0 == strcmp(argv[1], "-h"))
		{
			command = help;
			std::cerr << "TIP: help command takes no arguments." << std::endl;
		}
		else if (0 == strcmp(argv[1], "-u")) 
		{
			command = unsnag;
			target = argv[2];
		}
		else if (0 == strcmp(argv[1], "-a"))
		{
			command = add;
			target = argv[2];
		}
		else if (0 == strcmp(argv[1], "-d"))
		{
			command = del;
			target = argv[2];
		}
		else
		{
			std::cerr << "ERROR: invalid command '" << argv[1] << "', ";
			std::cerr << "Type snag -h for help" << std::endl;
			return 1;
		}
	}
	
	// perform commands
	
	if (command == help)
	{
		helpCommand();
		return 1;
	}

	Socket clientSocket = connectToServer();
	if (clientSocket == -1)
	{
		return 1;
	}
	char clientNameBuffer[20];
	getlogin_r(clientNameBuffer, 20);
	std::string clientName(clientNameBuffer);
	
	switch(command)
	{
		case list:
			listCommand(clientSocket);
			return 0;
			break;
		case snag:
			snagCommand(clientSocket, target, clientName);
			break;
		case unsnag:
			unsnagCommand(clientSocket, target, clientName);
			break;
		case add:
			addCommand(clientSocket, target, clientName);
			break;
		case del:
		  delCommand(clientSocket, target, clientName);
			break;
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

} // end main


void helpCommand()
{
	std::cout << std::endl;
	std::cout << "Snag is a lightweight client to book servers.\n\nUses" << std::endl;
	std::cout << "-------------" << std::endl;
	std::cout << "snag                 :  list the snaggables" << std::endl;
	std::cout << "snag <snaggable>     :  attempt to snag snaggable" << std::endl;
	std::cout << "snag -u <snaggable>  :  attempt to unsnag snaggable" << std::endl;
	std::cout << "snag -a <snaggable>  :  attempt to add snaggable" << std::endl;
	std::cout << "snag -d <snaggable>  :  attempt to delete snaggable" << std::endl;
	std::cout << std::endl;
}

void listCommand(Socket clientSocket)
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
	  return;
	}
	parseSnagList(buffer, readLen);
}

// TODO: THESE FUNCTIONS ARE BASICALLY THE SAME

void snagCommand(Socket clientSocket, std::string& target, std::string& clientName)
{
	std::ostringstream oss;
	oss << "s-" << target << "-" << clientName << "-";
	std::string message = oss.str();
	write(clientSocket, message.c_str(), message.size());
}

void unsnagCommand(Socket clientSocket, std::string& target, std::string& clientName)
{
	std::ostringstream oss;
	oss << "u-" << target << "-" << clientName << "-";
	std::string message = oss.str();
	write(clientSocket, message.c_str(), message.size());
}

void addCommand(Socket clientSocket, std::string& target, std::string& clientName)
{
	std::ostringstream oss;
	oss << "a-" << target << "-" << clientName << "-";
	std::string message = oss.str();
	write(clientSocket, message.c_str(), message.size());
}

void delCommand(Socket clientSocket, std::string& target, std::string& clientName)
{
	std::ostringstream oss;
	oss << "d-" << target << "-" << clientName << "-";
	std::string message = oss.str();
	write(clientSocket, message.c_str(), message.size());
}

Socket connectToServer()
{
  int portNumber = atoi("6640");
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

// TODO: make this not suck
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
