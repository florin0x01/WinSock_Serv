#include <winsock.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <map>
#include <process.h>

using namespace std;

SOCKET s;
WSADATA wsa;
HWND handle;
sockaddr_in s_in;
int addrlen = sizeof(sockaddr_in);
WNDPROC fnpW;

#define PORT 7891
#define MSG_PER_CLIENT 10

CRITICAL_SECTION CritSection;

vector<SOCKET> clients;
fd_set read_set, write_set, err_set;
map<SOCKET, int> per_client;

bool initWinsock()
{
	int error = WSAStartup(0x0202, &wsa);
	if (error) return false;
	if (wsa.wVersion != 0x0202) 
	{
		WSACleanup();
		return false;
	}

	return true;
}

void closeConn()
{
	if(s)
		closesocket(s);
	WSACleanup();
}

void DoSmthElse()
{

}

void AddClient(SOCKET client)
{
	clients.push_back(client);
	FD_SET(client, &read_set); FD_SET(client, &write_set); FD_SET(client, &err_set);
	per_client[client] = 0;
}

void ReleaseClient(SOCKET c)
{
	if(c) { closesocket(c); }
	per_client.erase(c);
	FD_CLR(c, &read_set);
	FD_CLR(c, &write_set);
	FD_CLR(c, &err_set);
	for(int i=0; i < clients.size(); i++) if(clients[i] == c) { clients.erase(clients.begin() + i); break; }
}

bool doWrite(SOCKET c)
{
	char buf[100];
	
	if(per_client[c] < MSG_PER_CLIENT)
	{
		memset(buf, 0, 100);
		sprintf(buf, "Hello peer %d !", c);
		cout << "Sending to client " << c << endl;
		if( send(c, buf, sizeof(buf), 0) <=0 ) return false;
		per_client[c]++;
	}
	return true;
}


bool doRead(SOCKET c)
{
	char buf[100];
	memset(buf,0,100);
	int size = recv(c, buf, 100, 0);
	if(size <= 0) return false;
	cout << "Received " << size << " bytes from client " << c << endl;
	cout << "->" << buf << "<-" << endl;
	//return doWrite(c);
	return true;
}


bool doErr(SOCKET c)
{
	cout << "Received error from connection " << c << ", closing " << endl;
	return false;
}

void handleSet(fd_set *set, bool (*doSmth)(SOCKET))
{
	for(vector<SOCKET>::iterator client = clients.begin(); client != clients.end(); )
	{
		if(FD_ISSET(*client, set))
		{
			if(!doSmth(*client)) 
			{
				cout << "Socket error from " << *client << ", closing " << endl;
				ReleaseClient(*client);
				client = clients.begin();
				continue;
			}
		}
		client++;
	}
}

bool MessagePump()
{
	struct timeval t1;
	t1.tv_sec = 2; t1.tv_usec = 0;
	int retSel = 0;

	struct sockaddr_in peer;
	int peerLen = sizeof(peer);
	
	FD_ZERO(&read_set); FD_ZERO(&write_set); FD_ZERO(&err_set);
	FD_SET(s, &read_set); FD_SET(s, &write_set); FD_SET(s, &err_set);

	for(unsigned i=0; i < clients.size(); i++) 
	{
		FD_SET(clients[i], &read_set);
		FD_SET(clients[i], &write_set);
		FD_SET(clients[i], &err_set);
	}

	retSel = select(s, &read_set, &write_set, &err_set, NULL);

	if (retSel == 0) { cout << "Timeout expired. " << endl; }
	if (retSel < 0 ) { cout << "Socket error " << WSAGetLastError() << endl; return false;}
	if (retSel)
	{
		handleSet(&read_set, doRead);
		handleSet(&write_set, doWrite);

		if(FD_ISSET(s, &read_set))
		{
			//accept the connection
			SOCKET client = accept(s, (sockaddr*)&peer, &peerLen);
			cout << "Accepted connection, client " << client << endl;
			AddClient(client);
		}

		
		//handleSet(&err_set, doErr);

	}
	DoSmthElse();
	return true;
}

void serve()
{
	if(!initWinsock())
	{
		cout << "Could not init winsock" << endl;
		return; 
	}

	InitializeCriticalSection(&CritSection);

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == SOCKET_ERROR) 
	{
		cout << "Socket error. " << endl;
		return;
	}

	//bind
	s_in.sin_family = AF_INET;
	s_in.sin_port = PORT;
	s_in.sin_addr.s_addr = inet_addr("0.0.0.0");

	if(bind(s, (sockaddr*)&s_in, sizeof(s_in))) { cout << "Bind returned error : " << WSAGetLastError() << endl; return ;}
	cout << "Socket bound " << endl;

	//listen
	if(listen(s, 5))
		cout << "listen error " << endl;
	else
		cout << "Listening on port " << PORT << endl;

	while(1)
	{
		if(!MessagePump()) break;
		Sleep(100);
	}
}

int main(int argc, char** argv)
{
	serve();
	closeConn();
	return 0;
}