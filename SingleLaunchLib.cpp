
#include "stdafx.h"
#include "SingleLaunchLib.h"


#include <stdexcept>
#include <handleapi.h>
//#include <ostream>
#include <iostream>
#include <wtypes.h>
#include <winsock.h>
#include <vector>
#include <string>
#include <thread>

using namespace std;

static const char SERVERADDR[16] = "255.255.255.255";
static const int portServer = 5150;
static const int portClient = 5250;

unsigned int CopiesTreshold;

#define FOREVER() for(;;)

bool bAcceptMessages = true;

std::vector<string> netClients;
std::vector<string> localClients;
static SOCKET server_sock, client_sock;
sockaddr_in out_addr;
sockaddr_in client_addr;

HANDLE mut;
HANDLE myHandle;
char buff[2048];
unsigned int counter = 1;
string hostName;
string senderName;
string ip;

namespace SingleLaunch
{

	SingleLaunch_Base::SingleLaunch_Base(unsigned int maxCopies) : MaxCopies(maxCopies)
	{
		/*mut = CreateMutex(nullptr, false, TEXT("TestMutex"));
		DWORD result;
		result = WaitForSingleObject(mut, 0);
		if (result == WAIT_OBJECT_0)
		{
		cout << "Start run" << endl;
		ReleaseMutex(mut);
		}
		else
		cout << "fail. program is tunning" << endl;*/


		cout << "Program is started...\n";
		//printf("UDP Server Started\n");

		CopiesTreshold = MaxCopies;

		// Inti WinSock


		out_addr = InitWinSocket(server_sock, portServer);
		mBindSocket(server_sock, out_addr, portServer);

		// Get local host name.
		char szHostName[128] = "";
		if (gethostname(szHostName, sizeof(szHostName)))
		{
			// Error handling
			printf("Get host error: %d\n", WSAGetLastError());
		}
		struct hostent* pHost = 0;
		pHost = gethostbyname(szHostName);
		hostName = szHostName;


		cout << "----------------\n";
		cout << "==>Send test msg.\n";
		cout << "----------------\n";

		char msg[20] = "ping";
		/*in_addr.sin_addr.s_addr = inet_addr(SERVERADDR);
		cout << "from port " << ntohs(in_addr.sin_port) << endl;
		sendto(server_sock, &msg[0], sizeof(msg), 0, (sockaddr*)&in_addr, sizeof(in_addr));
		printf("\nC=>S:%s\n", &msg[0]);*/
		sockaddr_in send_addr;
		send_addr.sin_family = AF_INET;
		send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		send_addr.sin_port = htons(portServer);
		send_addr.sin_addr.s_addr = inet_addr(SERVERADDR);

		cout << "from port " << ntohs(send_addr.sin_port) << endl;
		sendto(server_sock, &msg[0], sizeof(msg), 0, (sockaddr*)&send_addr, sizeof(send_addr));
		printf("\nC=>S:%s\n", &msg[0]);


		// make thread.
		std::thread  lisentThread(ThteadServerLis, server_sock, out_addr, portServer);
		lisentThread.detach();

	}

	sockaddr_in SingleLaunch_Base::InitWinSocket(SOCKET & out_socket, int porttoConnect)
{
		WORD wVersionRequested = MAKEWORD(2, 2);
		WSADATA wsaData;
		int err = WSAStartup(wVersionRequested, &wsaData);
		if (err != 0)
			printf("WSAStartup error: %d\n", WSAGetLastError());
		else
			printf("WinSock initializing\n");

		// Open/close socket.
		out_socket = socket(AF_INET, SOCK_DGRAM, 0);
		if (out_socket == INVALID_SOCKET)
		{
			printf("Socket error: %d\n", WSAGetLastError());
			WSACleanup();
		}

		int broadcast = 1;
		if (setsockopt(out_socket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof broadcast) != 0)
			cout << "Fail\n";

		// Связывание сокета 
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(porttoConnect);

		return addr;
	}

	int SingleLaunch_Base::mBindSocket(SOCKET sock, sockaddr_in addr, const int port)
	{
		int i = 0;
		if ((i = ::bind(sock, (sockaddr *)&addr, sizeof(addr))) == SOCKET_ERROR)
		{
			printf("bind error: %d\n", WSAGetLastError());
		}
		else
		{
			cout << "++++++++++++++++++++++++++" << endl;
			cout << "Socket successfully binded!" << endl;
			cout << "++++++++++++++++++++++++++" << endl;
		}
		return i;
	}

	void SingleLaunch::SingleLaunch_Base::ThteadClientLis(SOCKET sock, sockaddr_in addr, const int portID)
	{
	}

	void SingleLaunch::SingleLaunch_Base::ThteadServerLis(SOCKET sock, sockaddr_in addr, const int port)
	{
		FOREVER()
		{
			if (!bAcceptMessages)
				return;
			// Обработка присланных пактов
			//sockaddr_in client_addr;
			int client_addr_size = sizeof(client_addr);
			int buffSize = sizeof(buff);
			int bsize = recvfrom(sock, &buff[0], buffSize - 1, 0, (sockaddr *)&client_addr, &client_addr_size);

			if (bsize == SOCKET_ERROR)
				printf("recvfrom() error: %d\n", WSAGetLastError());
			else
				if (bAcceptMessages)
				{
					// Определяем IP-адрес клиента и прочие атрибуты
					HOSTENT *hostent;
					hostent = gethostbyaddr((char *)&client_addr.sin_addr, 4, AF_INET);
					ip = inet_ntoa(client_addr.sin_addr);

					if (hostent != nullptr)
						senderName = hostent->h_name;

					bool match = false;
					for (vector<string>::iterator it = netClients.begin(); it != netClients.end(); ++it)
					{
						if (*it == ip)
						{
							match = true;
							break;
						}
					}
					buff[bsize] = '\0';

					// NET PORTS.
					if (senderName != hostName &&
						match == false)
					{
						if (std::strcmp(buff, "ping") == 0)
						{
							netClients.push_back(ip);

							// Вывод на экран 
							printf("S=>C:%s\n", &buff[0]);

							cout << "Copies:";
							counter = netClients.size() + localClients.size();
							cout << counter << "\n destination ";
							cout << inet_ntoa(client_addr.sin_addr) << endl;
							cout << " from " << senderName.c_str() << endl;
							cout << " port " << ntohs(client_addr.sin_port) << endl;

							_itoa_s(counter, buff, 10);

							// посылка датаграммы клиенту.	
							for (vector<string>::iterator it = netClients.begin(); it != netClients.end(); ++it)
							{
								std::string intStr = *it;
								if (intStr != ip)
								{
									std::string s = std::to_string(std::stoi(*it));
									s = s + "\n";
									char const *pchar = s.c_str();
									/*in_addr.sin_port = htons(port);
									in_addr.sin_addr.s_addr = inet_addr(pchar);*/
									sendto(sock, &pchar[0], sizeof(pchar), 0, (sockaddr *)&client_addr, sizeof(client_addr));
								}
							}
						}


						std::string matchedStr("close");
						std::string str(buff);
						size_t position = str.find(matchedStr);
						if (position != string::npos)
						{
							cout << "---------------" << endl;
							cout << "net client exit" << endl;
							cout << "---------------" << endl;

							size_t result = str.find("_");

							if (result != string::npos)
							{
								std::string check;
								check.append(str.substr(result + 1));

								size_t elemId = 0;
								for (vector<string>::iterator it = netClients.begin(); it != netClients.end(); ++it)
								{
									if (*it == check)
									{
										break;
									}
									elemId++;
								}
								if (elemId < netClients.size())
								{
									netClients.erase(netClients.begin() + elemId);
								}
							}
							else
								cout << "fail to remove net player from arr!" << endl;
						}
					}
					// LOCAL PORTS.
					else
					{
						std::string localport = std::to_string(ntohs(client_addr.sin_port));

						for (vector<string>::iterator it = localClients.begin(); it != localClients.end(); ++it)
						{
							if (*it == localport)
							{
								match = true;
								break;
							}
						}

						std::string str(buff);
						std::string closeCommandStr("close_command");
						size_t position = str.find(closeCommandStr);
						if (position != string::npos)
						{
							// TODO:: Send message to the client to close it.
							cout << " You launch maximum copies count! - " << counter << endl;
							cout << " Q - to end session. " << counter << endl;
							char myChar = ' ';
							while (myChar != 'q') {

								myChar = getchar();
							}
							EndSession();

							return;
						}

						std::string closeStr("close");
						position = str.find(closeStr);
						if (position != string::npos)
						{
							cout << "--------------------" << endl;
							cout << "=> local client exit" << endl;
							cout << "--------------------" << endl;
							//  remove target client.
							size_t result = str.find("_");

							if (result != string::npos)
							{
								std::string check;
								check.append(str.substr(result + 1));

								size_t elemId = 0;
								for (vector<string>::iterator it = localClients.begin(); it != localClients.end(); ++it)
								{
									if (*it == check)
									{
										break;
									}
									elemId++;
								}
								if (elemId < localClients.size())
								{
									localClients.erase(localClients.begin() + elemId); // check the index.
								}

								CountClients();

								// Try to bind socket.

								/*closesocket(server_sock);

								InitWinSocket();
								in_addr.sin_addr.s_addr = inet_addr(SERVERADDR);
								mBindSocket(server_sock, in_addr, port);*/ // TODO:: udnarstand why this shit not rebind!
							}
							else
								cout << "fail to remove local player from arr!" << endl;
						}
						else if (match == false && std::strcmp(buff, "ping") == 0)
						{
							// Send message to the client to close it.
							if (counter >= CopiesTreshold)
							{
								addr.sin_port = client_addr.sin_port;
								addr.sin_addr.s_addr = inet_addr(SERVERADDR);
								sendto(sock, "close_command", sizeof("close_command"), 0, (sockaddr*)&out_addr, sizeof(out_addr));

							}
							else
							{
								localClients.push_back(localport);//ip

								cout << "local client launched" << endl;
								cout << " port " << ntohs(client_addr.sin_port) << endl;
								CountClients();

								char outMSG[2];
								_itoa_s(counter, outMSG, 10);

								// send for all locals.
								for (vector<string>::iterator it = localClients.begin(); it != localClients.end(); ++it)
								{
									int intStr = std::stoi(*it);
									if (intStr != port)
									{
										addr.sin_port = htons(intStr);
										addr.sin_addr.s_addr = inet_addr(SERVERADDR);

										for (vector<string>::iterator Kt = localClients.begin(); Kt != localClients.end(); ++Kt)
										{
											std::string s = std::to_string(std::stoi(*Kt));
											//s = s + "\n";
											const size_t len = s.size();
											char const *pchar = s.c_str();
											sendto(sock, &pchar[0], len, 0, (sockaddr*)&addr, sizeof(addr));
										}
									}
								}
							}

						}
						else
						{
							std::string incomStr = &buff[0];
							bool portMatch = false;
							if (localClients.size())
							{
								for (vector<string>::iterator it = localClients.begin(); it != localClients.end(); ++it)
								{
									if (*it == incomStr)
									{
										portMatch = true;
										break;
									}
								}
							}
							if (!portMatch)
							{
								localClients.push_back(incomStr);
							}
						}

						cout << "++++++++++++++++++++++++++" << endl;
						cout << " incoming: " << endl;
						cout << " port " << ntohs(client_addr.sin_port) << endl;
						cout << " port " << &buff[0] << endl;
						cout << "++++++++++++++++++++++++++" << endl;
						cout << "++++++++++++++++++++++++++" << endl;
						CountClients();
						cout << "++++++++++++++++++++++++++" << endl;
					}
				}
		}
	}

	int SingleLaunch::SingleLaunch_Base::CountClients()
	{
		counter = netClients.size() + localClients.size();
		cout << "=> Copies:" << "[local: " << localClients.size() << " ] " << "[net: " << netClients.size() << " ]" << endl;
		return counter;
	}

	void SingleLaunch::SingleLaunch_Base::EndSession()
	{
		bAcceptMessages = false;
		// local
		for (vector<string>::iterator it = localClients.begin(); it != localClients.end(); ++it)
		{
			int intStr = std::stoi(*it);
			//if (intStr != port)
			{
				std::cout << ' ' << intStr << endl;
				std::cout << ntohs(client_addr.sin_port) << endl;


				std::string s = "close";
				s.append("_");
				s.append(std::to_string(ntohs(client_addr.sin_port)));


				const size_t len = s.size();
				const char *pchar = s.c_str();

				out_addr.sin_port = htons(intStr);
				out_addr.sin_addr.s_addr = inet_addr(SERVERADDR);

				sendto(server_sock, &pchar[0], len
					, 0, (sockaddr*)&out_addr, sizeof(out_addr));
			}
		}

		// net
		for (vector<string>::iterator it = netClients.begin(); it != netClients.end(); ++it)
		{
			std::string intStr = *it;
			if (intStr != ip)
			{
				std::string s = "close";
				s.append("_");
				s.append(intStr);

				const size_t len = s.size();
				char const *pchar = intStr.c_str();

				out_addr.sin_port = htons(portServer);
				out_addr.sin_addr.s_addr = inet_addr(pchar);
				sendto(server_sock, &pchar[0], len, 0, (sockaddr*)&out_addr, sizeof(out_addr));
			}
		}

		WSACleanup();
		closesocket(server_sock);
	}

	SingleLaunch_Base::~SingleLaunch_Base()
	{
		cout << " --------------" << endl;
		cout << "> Close program <" << endl;
		cout << " --------------" << endl;

		if (server_sock != INVALID_SOCKET)
		{
			closesocket(server_sock);
			server_sock = INVALID_SOCKET;
		}

		ReleaseMutex(mut);
		CloseHandle(mut);
	}
}