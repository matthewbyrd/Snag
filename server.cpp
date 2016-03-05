// C libraries for sockets and threads
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

// C++ libraries
#include <iostream>
#include <vector>
#include <algorithm>

// Make the C socket stuff more pleasant
#define IPV4 AF_INET
#define TCP SOCK_STREAM
const uint8_t defaultProtocol = 0;
const uint8_t maxPendingConnections = 5;
const uint8_t readSize = 255;
const uint8_t maxNumThreads = 10;
typedef int Socket;
typedef struct sockaddr_in SocketAddress;

const uint8_t maxConnectionsAllowed = 10;

pthread_mutex_t clientListMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t threadListMutex = PTHREAD_MUTEX_INITIALIZER;

struct Client
{
	std::string name;
	Socket socket = -1;
	int threadID = -1;
	// std::vector<Snaggable> snaggedItems;
	
	bool operator== (const Client& rhs)
	{
	  return 
	    socket == rhs.socket &&
		  threadID == rhs.threadID;
	}
};

static std::vector<Client> connectedClients;
const bool isTaken = true;
const bool isFree = false;
static bool takenThreads[maxConnectionsAllowed];

//TODO
/*

[√] MUST MUTEX LOCK THE CLIENT VECTOR DURING OPERATIONS!

[ ] OOP because Bjarne

[√] deal with thread list -- maybe track free thread IDs?

[ ] use getline instead of fgets...

[√] sort out the passing of messages -- currently superfluously vectors?

[ ] keep track of list of snaggables (and ping them, broadcast their status)

[ ] rejig the client to send heartbeats and get acknowledgements

[ ] Allows clients to add new snaggables and to snag and unsnag snaggables

*/

int allocateThreadID()
{
	for (auto i = 0; i < maxConnectionsAllowed; ++i)
	{
		if (takenThreads[i] == isFree)
		{
			takenThreads[i] = isTaken;
			return i;
		}
	}
	return -1;
}

void freeThreadID(uint8_t id)
{
	if (takenThreads[id] == isTaken)
	{
		takenThreads[id] = isFree;
	}
	else
	{
		std::cerr << "ERROR: Trying to free already free thread ID" << std::endl;
	}
}

void forgetClient(Client& client)
{
	pthread_mutex_lock(&clientListMutex);
	
	connectedClients.erase
	(
		std::remove(connectedClients.begin(), 
	              connectedClients.end(), 
								client), 
		connectedClients.end()
	);
	
	pthread_mutex_unlock(&clientListMutex);	
}

bool message(Client& client, uint8_t* data, size_t dataLen)
{
  int writeLen = write(client.socket, data, dataLen);
  
  if (writeLen < dataLen)
  {
	  return false;
  }
  return true;
}

bool message(Client& client, std::string text)
{
  int writeLen = write(client.socket, &text[0], text.size());
  
  if (writeLen < text.size())
  {
	  return false;
  }
  return true;
}

void broadcast(uint8_t* data, size_t dataLen)
{
	for (auto client : connectedClients)
	{
		message(client, data, dataLen);
	}
}

void broadcastToAllExcept(uint8_t* data, size_t dataLen, Client& excludingClient)
{
	for (auto client : connectedClients)
	{
		if (client.socket != excludingClient.socket)
		{
			message(client, data, dataLen);
		}
	}
}

Socket getClientSocket(Socket welcomingSocket)
{
  SocketAddress clientAddress;
  socklen_t clientAddressLen = sizeof(clientAddress);
  Socket clientSocket = accept(welcomingSocket, (struct sockaddr *) &clientAddress, &clientAddressLen);
 
  return clientSocket;
}

int outputErrorAndQuit(std::string errorMessage)
{
	std::cerr << errorMessage << std::endl;
	return 1;
}

Socket setupAndListen(int port)
{
    Socket welcomingSocket = socket(IPV4, TCP, defaultProtocol);
    if (welcomingSocket < 0) 
 	{
       std::cerr << "ERROR opening welcoming-socket" << std::endl;
	   return -1;
 	}
 
	SocketAddress serverAddress;
 	memset((char *)&serverAddress, 0, sizeof(serverAddress));
    int portNumber = port;
    serverAddress.sin_family = IPV4;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(portNumber);
    if (bind(welcomingSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) 
 	{
        std::cerr << "ERROR : failed to bind welcoming-socket" << std::endl;
		return -1;
    }
 
    listen(welcomingSocket, maxPendingConnections);
 	std::cout << "Listening on port "<< portNumber << ". . . " << std::endl;
 
 	return welcomingSocket;
}

/* void pointer mlarkey is due to calling with pthread_create */
void* mainClientLoop(void* clientptr)
{
	Client& client = *( (Client*) clientptr);
	uint8_t buffer[readSize];
	
	std::cout << "speaking to client on socket: " << client.socket
		<< " with thread id: " << client.threadID << std::endl;
	
	while (1)
	{
		memset(buffer, 0, readSize);
		
		std::cout << "waiting for message" << std::endl;
    int readLen = read(client.socket, buffer, readSize);
		std::cout << "Read something of size: " << readLen << std::endl;
		
		if (readLen <= 0)
		{
	    std::cout << "Client disconnected." << std::endl;
	    close(client.socket);
			pthread_mutex_lock(&clientListMutex);
			forgetClient(client);
			pthread_mutex_unlock(&clientListMutex);
			pthread_mutex_lock(&threadListMutex);
			freeThreadID(client.threadID);
			pthread_mutex_unlock(&threadListMutex);
	    pthread_exit(NULL);
		}
		
		
		std::cout << "Client wrote a message: " << buffer << std::endl;
		broadcastToAllExcept(buffer, readLen, client);
		message(client, "got it");
	}
}

int main (int argc, char *argv[])
{
  if (argc < 2)
	{
    std::cerr << "USAGE: " << argv[0] << " port" << std::endl;
    return 1;
  }
	 
	// create 10 handles for the threads
	pthread_t threads[maxNumThreads];
	 
	Socket welcomingSocket = setupAndListen(atoi(argv[1]));
	if (welcomingSocket == -1)
  {
    return outputErrorAndQuit("ERROR : failed to open welcoming socket");
  }
	 
  while (1)
  {
		Client newClient;
		
		// set up client socket
	  newClient.socket = getClientSocket(welcomingSocket);
		pthread_mutex_lock(&clientListMutex);
		if (newClient.socket >= 0 && connectedClients.size() >= maxConnectionsAllowed)
		{
			std::cerr << "ERROR: Too many clients trying to connect." << std::endl;
			close(newClient.socket);
			continue;
		}
		pthread_mutex_unlock(&clientListMutex);
	 
		// TODO: set up client name
		bool sentInitialMessage = message(newClient, "Welcome to snag, client. Here is some helpful information:\n");
		if (!sentInitialMessage) 
		{
		  std::cerr << "ERROR: Failed to send initial message";
		}
		
		// set up client thread
		pthread_mutex_lock(&threadListMutex);
		newClient.threadID = allocateThreadID();
		if (newClient.threadID < 0)
		{
			std::cerr << "ERROR: Cannot allocate client thread" << std::endl;
		}
		pthread_mutex_lock(&clientListMutex);
		connectedClients.push_back(newClient);
		void* client = &connectedClients[connectedClients.size()-1];
		pthread_create(&threads[newClient.threadID], NULL, mainClientLoop, client);
		pthread_mutex_unlock(&threadListMutex);
		pthread_mutex_unlock(&clientListMutex);
  }
	
  close(welcomingSocket);
	 
  return 0;
}