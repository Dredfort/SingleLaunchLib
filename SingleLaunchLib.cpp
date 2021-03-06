
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
sockaddr_in client_in_addr;

sockaddr_in out_addr2;
sockaddr_in client_in_addr2;

std::string localPort;

int serverBind = -1;

HANDLE mut;

HANDLE hMutex;

HANDLE myHandle;
char buff[2048];
unsigned int counter = 0;
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
		out_addr2 = InitWinSocket(client_sock, portClient);
		int clientBind = mBindSocket(client_sock, out_addr2, portClient);




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



		hMutex = CreateMutex(NULL, FALSE, NULL);
		// make thread.
		std::thread clientThr(ThteadClientLis, client_sock, out_addr2, portClient);
		clientThr.detach();

		if (clientBind == 0)
		{
			out_addr = InitWinSocket(server_sock, portServer);
			serverBind = mBindSocket(server_sock, out_addr, portServer);

			if (serverBind == 0)
			{
				std::thread  lisentThread(ThteadServerLis, server_sock, out_addr, portServer);
				lisentThread.detach();
			}
			else
				closesocket(server_sock);

		}

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

		// ���������� ������ 
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
			cout << "\n++++++++++++++++++++++++++" << endl;
			cout << "Socket successfully binded!" << endl;
			cout << "++++++++++++++++++++++++++" << endl;
		}
		return i;
	}

	void SingleLaunch::SingleLaunch_Base::ThteadClientLis(SOCKET sock, sockaddr_in addr, const int portID)
	{
		WaitForSingleObject(hMutex, INFINITE);

		cout << "\n-----------------------------" << endl;
		cout << "C:==> Try to ping the server." << endl;
		cout << "-----------------------------" << endl;

		char msg[20] = "ping";
		sockaddr_in send_addr;
		send_addr.sin_family = AF_INET;
		send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		send_addr.sin_port = htons(portServer);
		send_addr.sin_addr.s_addr = inet_addr(SERVERADDR);

		//cout << "from port " << ntohs(send_addr.sin_port) << endl;
		sendto(sock, &msg[0], sizeof(msg), 0, (sockaddr*)&send_addr, sizeof(send_addr));
		//printf("\nC=>S:%s\n", &msg[0]);
		cout << "\nC:==> Start lisent port." << endl;

		FOREVER()
		{
			if (!bAcceptMessages)
				return;
			// ��������� ���������� ������
			//sockaddr_in client_addr;
			int client_addr_size = sizeof(client_in_addr2);
			int buffSize = sizeof(buff);

			ReleaseMutex(hMutex);

			int bsize = recvfrom(sock, &buff[0], buffSize - 1, 0, (sockaddr *)&client_in_addr2, &client_addr_size);
			if (bsize == SOCKET_ERROR)
			{
				printf("recvfrom() error: %d\n", WSAGetLastError());

				closesocket(sock);
				ReleaseMutex(hMutex);
				ExitThread(0);
			}
			else
			{

				if (serverBind == -1)
				{
					cout << "\n++++++++++++++++++++++++++" << endl;
					cout << "C:> incoming: " << endl;
					cout << "C:> port " << ntohs(client_in_addr2.sin_port) << endl;
					cout << "C:> message: " << &buff[0] << endl;
					cout << "++++++++++++++++++++++++++" << endl;
				}

				// TODO:: get and accept messages here
				std::string buffStr(buff);

				std::string pingClientsStr("ping_clients");
				std::string closeCommandStr("close_command");
				std::string closeStr("closed");

				size_t	findClose = buffStr.find(closeStr);
				size_t findCloseCommand = buffStr.find(closeCommandStr);
				size_t findPingClients = buffStr.find(pingClientsStr);

				std::string qwe = std::to_string(portClient);

				size_t pos = buffStr.find(qwe);


				if (findPingClients != string::npos)
				{
					cout << "\n-------------------------------------" << endl;
					cout << "C:==> Send `ping` response to server." << endl;
					cout << "-------------------------------------" << endl;
					sendto(sock, &msg[0], sizeof(msg), 0, (sockaddr*)&send_addr, sizeof(send_addr));
				}
				else if (findCloseCommand != string::npos)
				{
					std::string check;
					check.append(buffStr.substr(findCloseCommand + 1));
					// TODO:: Send message to the client to close it.
					cout << "\n You launch maximum copies count! - " << check << endl;
					cout << " Q - to end session. " << endl;
					char myChar = ' ';
					while (myChar != 'q') {

						myChar = getchar();
					}

					//EndSession();

					std::cout << ntohs(client_in_addr.sin_port) << endl;


					std::string s = "closed";
					s.append("_");
					s.append(std::to_string(ntohs(client_in_addr.sin_port)));


					const size_t len = s.size();
					const char *pchar = s.c_str();

					sendto(sock/*server_sock*/, &pchar[0], len
						, 0, (sockaddr*)&send_addr, sizeof(send_addr));

					return;
				}
				else if (findClose != string::npos)
				{
					WaitForSingleObject(hMutex, INFINITE);

					cout << "\n--------------------------------------------------" << endl;
					cout << "C:==> Server is exit. Try to create server thread." << endl;
					cout << "--------------------------------------------------" << endl;
					Sleep(3000);


					out_addr = InitWinSocket(server_sock, portServer);
					serverBind = mBindSocket(server_sock, out_addr, portServer);

					ReleaseMutex(hMutex);

					if (serverBind == 0)
					{
						cout << "\nC:> Starts to create server thread..." << endl;
						std::thread  lisentThread(ThteadServerLis, server_sock, out_addr, portServer);
						lisentThread.detach();
					}
					else
					{
						closesocket(server_sock);
						cout << "\n-----------------------------------" << endl;
						cout << "\nC:==> fail create server thread.." << endl;
						cout << "-----------------------------------" << endl;
					}
				}
				else if (pos == string::npos /*&& localPort.length() == 0*/)
				{
					localPort = buffStr;
				}
			}
		}

	}

	void SingleLaunch::SingleLaunch_Base::ThteadServerLis(SOCKET sock, sockaddr_in addr, const int port)
	{

		cout << "\n+++++++++++++++++++++++++++++++++++++++" << endl;
		cout << "S:==> ServerCreated! <== \n S:>Send ping request to all clients." << endl;
		cout << "++++++++++++++++++++++++++++++++++++++" << endl;

		char msg[20] = "ping_clients";
		sockaddr_in send_addr;
		send_addr.sin_family = AF_INET;
		send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		send_addr.sin_addr.s_addr = inet_addr(SERVERADDR);

		//send_addr.sin_port = htons(portClient);

		cout << "S:==> start picking clients."<< endl;
		for (auto i = 49152; i <= 65535; i++)
		{
			send_addr.sin_port = htons(i);

			sendto(sock/*server_sock*/, &msg[0], sizeof(msg), 0, (sockaddr*)&send_addr, sizeof(send_addr));
		}
		cout << "\nS:> end picking clients." << endl;
		cout << "\nS:> Start lisent port." << endl;
		FOREVER()
		{


			WaitForSingleObject(hMutex, INFINITE);

			//Sleep(1000);

			if (!bAcceptMessages)
				return;
			// ��������� ���������� ������			
			int client_addr_size = sizeof(client_in_addr);
			int buffSize = sizeof(buff);
			int bsize = recvfrom(sock, &buff[0], buffSize - 1, 0, (sockaddr *)&client_in_addr, &client_addr_size);

			if (bsize == SOCKET_ERROR)
			{
				printf("recvfrom() error: %d\n", WSAGetLastError());
				closesocket(sock);
				ReleaseMutex(hMutex);
				ExitThread(0);
			}
			else
				if (bAcceptMessages)
				{
					// ���������� IP-����� ������� � ������ ��������
					HOSTENT *hostent;
					hostent = gethostbyaddr((char *)&client_in_addr.sin_addr, 4, AF_INET);
					ip = inet_ntoa(client_in_addr.sin_addr);

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

					//// NET PORTS.
					if (senderName != hostName &&
						match == false)
					{
						if (std::strcmp(buff, "ping") == 0)
						{
							netClients.push_back(ip);

							// ����� �� ����� 
							printf("S=>C:%s\n", &buff[0]);

							/*cout << "Copies:";
							counter = netClients.size() + localClients.size();*/
							//CountClients();
							cout << /*counter <<*/ "\n destination ";
							cout << inet_ntoa(client_in_addr.sin_addr) << endl;
							cout << " from " << senderName.c_str() << endl;
							cout << " port " << ntohs(client_in_addr.sin_port) << endl;

							_itoa_s(counter, buff, 10);

							// ������� ���������� �������.	
							for (vector<string>::iterator it = netClients.begin(); it != netClients.end(); ++it)
							{
								std::string intStr = *it;
								if (intStr != ip)
								{
									std::string s = std::to_string(std::stoi(*it));
									s = s + "\n";
									char const *pchar = s.c_str();
									sendto(sock, &pchar[0], sizeof(pchar), 0, (sockaddr *)&client_in_addr, sizeof(client_in_addr));
								}
							}
						}


						std::string matchedStr("closed");
						std::string str(buff);
						size_t position = str.find(matchedStr);
						if (position != string::npos)
						{
							cout << "\n---------------" << endl;
							cout << "net client exit" << endl;
							cout << "---------------" << endl;

							/*size_t result = str.find("_");

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
								cout << "fail to remove net player from arr!" << endl;*/
						}
					}
	//// LOCAL PORTS.
					else
					{
						std::string localport = std::to_string(ntohs(client_in_addr.sin_port));

						for (vector<string>::iterator it = localClients.begin(); it != localClients.end(); ++it)
						{
							if (*it == localport)
							{
								match = true;
								break;
							}
						}

						std::string buffStr(buff);
						std::string closeStr("closed");
						std::string pingString("ping");

						size_t findCloseStr = buffStr.find(closeStr);
						size_t findPingStr = buffStr.find(pingString);

						if (findCloseStr != string::npos)
						{
							cout << "\n-----------------------" << endl;
							cout << "S:==> local client exit" << endl;
							cout << "-----------------------" << endl;

							counter--;

							CountClients();
							//  remove target client.
							//size_t result = buffStr.find("_");

							//if (result != string::npos)
							//{
							//	std::string check;
							//	check.append(buffStr.substr(result + 1));

							//	size_t elemId = 0;
							//	for (vector<string>::iterator it = localClients.begin(); it != localClients.end(); ++it)
							//	{
							//		if (*it == check)
							//		{
							//			break;
							//		}
							//		elemId++;
							//	}
							//	if (elemId < localClients.size())
							//	{
							//		localClients.erase(localClients.begin() + elemId); // check the index.
							//	}

							//}
							//else
							//	cout << "S:==>fail to remove local player from arr!" << endl;
						}
						else if (match == false && findPingStr != string::npos)// (match == false && std::strcmp(buff, "ping") == 0)
						{
							counter++;

							// Send message to the client to close it.
							if (counter > CopiesTreshold)
							{
								cout << "\nS:==> launched [" << counter << "] copies of application with limit [" << CopiesTreshold << "]" << endl;
								
								addr.sin_port = client_in_addr.sin_port;
								addr.sin_addr.s_addr = inet_addr(SERVERADDR);

								std::string stringMsg = "close_command";
								stringMsg.append("_");
								stringMsg.append(std::to_string(counter));

								const size_t len = stringMsg.size();
								const char *pchar = stringMsg.c_str();

								sendto(sock, pchar, len, 0,
									(sockaddr*)&client_in_addr/*out_addr*/,
									sizeof(client_in_addr/*out_addr*/));
							}
							else
							{
								//localClients.push_back(localport);//ip

								cout << "\n==========================" << endl;
								cout << "S:==>local client launched" << endl;
								cout << "S:> port " << ntohs(client_in_addr.sin_port) << endl;
								//CountClients();
								cout << "==========================" << endl;

								char outMSG[2];
								_itoa_s(counter, outMSG, 10);


								// send for all locals.
								//for (vector<string>::iterator it = localClients.begin(); it != localClients.end(); ++it)
								//{
								//	int intStr = std::stoi(*it);
								//	if (intStr != port)
								//	{
								//		addr.sin_port = htons(intStr);
								//		addr.sin_addr.s_addr = inet_addr(SERVERADDR);

								//		for (vector<string>::iterator Kt = localClients.begin(); Kt != localClients.end(); ++Kt)
								//		{
								//			std::string s = std::to_string(std::stoi(*Kt));
								//			//s = s + "\n";
								//			const size_t len = s.size();
								//			char const *pchar = s.c_str();
								//			sendto(sock, &pchar[0], len, 0, (sockaddr*)&addr, sizeof(addr));
								//		}
								//	}
								//}
							}
							CountClients();

						}
					}
				}

			ReleaseMutex(hMutex);
		}
	}

	int SingleLaunch::SingleLaunch_Base::CountClients()
	{
		//counter = netClients.size() + localClients.size();
		cout << "=> Copies:" << counter/*"[local: " << localClients.size() << " ] " << "[net: " << netClients.size() << " ]"*/ << endl;
		return counter;
	}

	void SingleLaunch::SingleLaunch_Base::EndSession()
	{
		bAcceptMessages = false;

		std::string s = "closed";
		s.append("_");
		s.append(localPort);
		const size_t len = s.size();
		const char *pchar = s.c_str();

		if (serverBind == -1)// client closed.
		{
			sockaddr_in send_addr;
			send_addr.sin_family = AF_INET;
			send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
			send_addr.sin_addr.s_addr = inet_addr(SERVERADDR);
			send_addr.sin_port = htons(portServer);

			sendto(client_sock/*server_sock*/, &pchar[0], len
				, 0, (sockaddr*)&send_addr, sizeof(send_addr));
		}
		else // server closed.
		{
			sockaddr_in send_addr;
			send_addr.sin_family = AF_INET;
			send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
			send_addr.sin_addr.s_addr = inet_addr(SERVERADDR);

			for (auto i = 49152; i <= 65535; i++)
			{
				send_addr.sin_port = htons(i);

				sendto(server_sock, &pchar[0], len, 0, (sockaddr*)&send_addr, sizeof(send_addr));
			}
		}

		// TODo:: send msg to all clients that server is closed.

		// local
		// for (vector<string>::iterator it = localClients.begin(); it != localClients.end(); ++it)
		// {
			// int intStr = std::stoi(*it);
			// if (intStr != portServer)
			// {
				// std::cout << ' ' << intStr << endl;
				// std::cout << ntohs(client_in_addr.sin_port) << endl;

				// out_addr.sin_port = htons(intStr);
				// out_addr.sin_addr.s_addr = inet_addr(SERVERADDR);

				// sendto(server_sock, &pchar[0], len
					// , 0, (sockaddr*)&out_addr, sizeof(out_addr));
			// }
			// //Sleep(1000);
		// }

		// net
		/*for (vector<string>::iterator it = netClients.begin(); it != netClients.end(); ++it)
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
		}*/
		Sleep(1000);
		WSACleanup();
		//closesocket(server_sock);
	}

	SingleLaunch_Base::~SingleLaunch_Base()
	{
		closesocket(client_sock);
		closesocket(server_sock);

		cout << " --------------" << endl;
		cout << "> Close program <" << endl;
		cout << " --------------" << endl;

		if (server_sock != INVALID_SOCKET)
		{
			server_sock = INVALID_SOCKET;
		}
		if (client_sock != INVALID_SOCKET)
		{
			client_sock = INVALID_SOCKET;
		}

		ReleaseMutex(hMutex);
		CloseHandle(hMutex);
		ReleaseMutex(mut);
		CloseHandle(mut);
	}
}