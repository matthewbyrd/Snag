// C libraries for sockets and threads
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

// C++ libraries
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>

// Make the C socket stuff more pleasant
#define IPV4 AF_INET
#define TCP SOCK_STREAM
const uint8_t defaultProtocol = 0;
typedef int Socket;
typedef struct sockaddr_in SocketAddress;

// Constants
const uint8_t maxPendingConnections = 5;
const uint8_t readSize = 255;

// prototypes
bool message(Socket client, uint8_t* data, size_t dataLen);
bool message(Socket client, std::string text);
Socket setupAndListen(int port);
Socket getClientSocket(Socket welcomingSocket);
int outputErrorAndQuit(const char* errorMessage);
std::string secondsToTime(double totalSecondsDouble);
std::string extractMachineName(uint8_t* message, int messageLen);
std::string extractUserName(uint8_t* message, int messageLen);

struct Machine;
std::vector<Machine> snaggables;

struct Machine
{
	static uint8_t id;
	
	Machine (std::string name)
		: m_name(name)
		, m_id(id++)
	  , m_snagged(false)
	  , m_status(true)
		
	{
		snaggables.push_back(*this);
	}
	
	std::string m_name;
	uint8_t m_id;
	bool m_snagged;
	time_t m_snaggedTime;
	bool m_status;
	std::string m_snagger;
	//ip
	
  bool operator== (const Machine& rhs)
  {
    return 
      m_name == rhs.m_name &&
      m_id == rhs.m_id;
  }
};

uint8_t Machine::id = 0;

// for sort
bool operator<(Machine const & a, Machine const & b)
{
    return a.m_name < b.m_name;
}

//----------------------------------------------------------------------------


int main (int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "USAGE: " << argv[0] << " port" << std::endl;
    return 1;
  }
	
	Machine dc1("DC1");
	Machine dc2("DC2");
	Machine dc4("DC4");
	Machine dc5("DC5");
	Machine devperf29("DEVPERF29");
	Machine devperf30("DEVPERF30");
	Machine devperf31("DEVPERF31");
	
  Socket welcomingSocket = setupAndListen(atoi(argv[1]));
  if (welcomingSocket == -1)
  {
    return outputErrorAndQuit("ERROR : failed to open welcoming socket");
  }
	
	while (1)
	{
		Socket client = getClientSocket(welcomingSocket);
		
	  uint8_t buffer[readSize];
		memset(buffer, 0, readSize);
		
		int readLen = read(client, buffer, readSize);
    if (readLen <= 0)
    {
      std::cout << "Client failed to connect." << std::endl;
	  }
		
		if (buffer[0] == 'a')
		{
			std::string requestedMachine = extractMachineName(buffer, readLen);
			if (requestedMachine.size() > 22)
			{
				message(client, "Machine name too long");
				continue;
			}
			else if (requestedMachine.find('#') != std::string::npos)
			{
				message(client, "Machine name must not include '#' because my creator sucks");
				continue;
			}
			for (auto & c: requestedMachine) c = toupper(c);
			bool machineExists = false;
			for (Machine& machine : snaggables)
			{
				if (machine.m_name == requestedMachine)
				{
					message(client, "That machine already exists");
					machineExists = true;
					break;
				}
			}
			if (!machineExists)
			{
				Machine newMachine(requestedMachine);
				message(client, "Machine successfully created");
			}
		}
		else if (buffer[0] == 'd')
		{
			std::string requestedMachine = extractMachineName(buffer, readLen);
			for (auto & c: requestedMachine) c = toupper(c);
			std::string user = extractUserName(buffer, readLen);
			bool machineExists = false;
			for (Machine& machine : snaggables)
			{
				if (machine.m_name == requestedMachine)
				{
					machineExists = true;
					if (machine.m_snagged && machine.m_snagger == user)
					{
					  snaggables.erase(std::remove(snaggables.begin(), snaggables.end(), machine), snaggables.end());
						message(client, "The machine has been successfully deleted");
						break;
					}
					else
					{
						message(client, "You can only delete a machine you have snagged");
						break;
					}
				}
			}
			if (!machineExists)
			{
				message(client, "You cannot delete a non-existent machine");
			}
		}
		else if (buffer[0] == 'l')
		{
			time_t now = time(NULL);
			std::sort(snaggables.begin(), snaggables.end());
			std::ostringstream oss;
			for (auto machine : snaggables)
      {
				oss << "#";            // <--- TODO: implement some basic protocol for sending this rather than relying on #
				oss << machine.m_name;
				oss << "#";
				oss << machine.m_snagged;
				oss << "#";
				if (machine.m_snagged)
				{
				  oss << machine.m_snagger;
					oss << "#";
					double seconds = difftime(now, machine.m_snaggedTime);
				  oss << secondsToTime(seconds);
					oss << "#";
			  }
      }
			message(client, oss.str());
		}
		else if (buffer[0] == 's')
		{
			std::string requestedMachine = extractMachineName(buffer, readLen);
			for (auto & c: requestedMachine) c = toupper(c);
			//int requestedMachineAsInt = atoi(requestedMachine.c_str());
			std::string user = extractUserName(buffer, readLen);
			bool machineExists = false;
			for (Machine& machine : snaggables)
			{
				if (machine.m_name == requestedMachine)
				{
					if (!machine.m_snagged)
					{
						machine.m_snagged = true;
						machine.m_snagger = user;
						machine.m_snaggedTime = time(NULL);
						std::ostringstream reply;
						reply << "Machine " << machine.m_name << " successfully snagged.";
						message(client, reply.str());
						machineExists = true;
						break;
					}
					else if (machine.m_snagged && machine.m_snagger == user)
					{
						std::ostringstream reply;
						reply << "Machine " << machine.m_name << " is already snagged by you, silly!";
						message(client, reply.str());
						machineExists = true;
						break;
					}
					else
					{
						std::ostringstream reply;
						reply << "Machine " << machine.m_name << " is already snagged by ." << machine.m_snagger;
						message(client, reply.str());
						machineExists = true;
						break;
					}
				}
			}
			if (!machineExists)
			{
				std::ostringstream reply;
				reply << "Machine " << requestedMachine << " does not exist.";
				message(client, reply.str());
			}
		}
		else if (buffer [0] == 'u')
		{
			std::string requestedMachine = extractMachineName(buffer, readLen);
			for (auto & c: requestedMachine) c = toupper(c);
			std::string user = extractUserName(buffer, readLen);
			bool machineExists = false;
			for (Machine& machine : snaggables)
			{
				if (machine.m_name == requestedMachine)
				{
					machineExists = true;
					if (machine.m_snagged && machine.m_snagger == user)
					{
						machine.m_snagged = false;
						machine.m_snagger = "";
						std::ostringstream reply;
						reply << "Machine " << machine.m_name << " successfully unsnagged.";
						message(client, reply.str());
						break;
					}
					else
					{
						std::ostringstream reply;
						reply << "Machine " << machine.m_name << " is not snagged by you.";
						message(client, reply.str());
						break;
					}
				}
			}
			if (!machineExists)
			{
				std::ostringstream reply;
				reply << "Machine " << requestedMachine << " does not exist.";
				message(client, reply.str());
		  }
		}
		
		close(client);
	}
	
	close(welcomingSocket);
	
}

bool message(Socket client, uint8_t* data, size_t dataLen)
{
  uint32_t writeLen = write(client, data, dataLen);
  
  if (writeLen < dataLen)
  {
    return false;
  }
  return true;
}

bool message(Socket client, std::string text)
{
  uint32_t writeLen = write(client, &text[0], text.size());
  
  if (writeLen < text.size())
  {
    return false;
  }
  return true;
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

Socket getClientSocket(Socket welcomingSocket)
{
  SocketAddress clientAddress;
  socklen_t clientAddressLen = sizeof(clientAddress);
  Socket clientSocket = accept(welcomingSocket, (struct sockaddr *) &clientAddress, &clientAddressLen);
 
  return clientSocket;
}

int outputErrorAndQuit(const char* errorMessage)
{
  std::cerr << errorMessage << std::endl;
  return 1;
}

std::string secondsToTime(double totalSecondsDouble)
{
	std::ostringstream oss;
	uint64_t totalSeconds = totalSecondsDouble;
	int seconds, hours, minutes;
	minutes = totalSeconds / 60;
	hours = minutes / 60;
	seconds = (int)(totalSeconds % 60);
	if (hours)
	{
	  oss << hours;
	  oss << "h ";
  }
	if (minutes)
	{
	  oss << minutes;
	  oss << "m ";
	}
	oss << seconds;
	oss << "s ";
	std::string display;
	display += oss.str();
	return display;
}

// TODO: make these two functions not suck -- use regex? 
std::string extractMachineName(uint8_t* message, int messageLen)
{
	std::string machine;
	for (int i = 2; message[i] != '-' && i < messageLen; ++i)
	{
		machine += message[i];
	}
	return machine;
}

std::string extractUserName(uint8_t* message, int messageLen)
{
	std::string user;
	// skip the machine name
	int i = 2; 
	for (; message[i] != '-' && i < messageLen; ++i){}
	++i;
	
	// get name
	for (; message[i] != '-' && i < messageLen; ++i)
	{
		user += message[i];
	}
	return user;
}
