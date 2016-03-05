// C libraries for sockets
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

// C++ libraries
#include <iostream>

// Make the C socket stuff more pleasant
#define IPV4 AF_INET
#define TCP SOCK_STREAM
const uint8_t defaultProtocol = 0;
typedef struct sockaddr_in SocketAddress;
typedef struct hostent Host;  

int main (int argc, char *argv[])  
{
  if (argc < 3) 
  {
    std::cerr << "USAGE: " << argv[0] << " hostname port" << std::endl;
    return 1;
  }
  
  int portNumber = atoi(argv[2]);
  int clientSocket = socket(IPV4, SOCK_STREAM, defaultProtocol);
  if (clientSocket < 0) 
  {
    std::cerr << "ERROR opening socket" << std::endl;
    return 1;
  }
  
  Host* server = gethostbyname(argv[1]);
  if (server == NULL)
  {
    std::cerr << "ERROR, no such host " << std::endl;
    return 1;
  }
  
  SocketAddress serverAddress;
  memset((char*)&serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin_family = IPV4;
  memcpy((char *)&serverAddress.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
  serverAddress.sin_port = htons(portNumber);
  if (connect(clientSocket,(const sockaddr*)&serverAddress,sizeof(serverAddress)) < 0)
  {
    std::cerr << "ERROR connecting" << std::endl;
    return 1;
  }
	
  char buffer[256];
  memset(buffer, 0, 256);
  int readLen = read(clientSocket, buffer, 255);
  if (readLen < 0)
  {
    std::cerr << "ERROR reading from socket" << std::endl;
    return 1;
  }
	std::cout << buffer << std::endl;
  
  while (1)
  {
    std::cout << "Please enter the message: " << std::endl;
    memset(buffer, 0, 256);
    fgets(buffer, 255, stdin);
		
    int writeLen = write(clientSocket, buffer, strlen(buffer));
    if (writeLen < 0) 
    {
      std::cerr << "ERROR writing to socket" << std::endl;
      return 1;
    }
				
    memset(buffer, 0, 256);
    int readLen = read(clientSocket, buffer, 255);
    if (readLen < 0)
    {
      std::cerr << "ERROR reading from socket" << std::endl;
      return 1;
    }
    
		std::cout << "Reply from server: " << buffer << std::endl;
		
  }
	
    close(clientSocket);
    return 0;
}