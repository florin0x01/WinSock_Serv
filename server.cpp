#define _WIN32_WINNT 0x05000

#include <winsock.h>
#include <windows.h>
#include <iostream>
#include <process.h>

using namespace std;

SOCKET s;
WSADATA wsa;
HWND handle;
sockaddr_in s_in;
int addrlen = sizeof(sockaddr_in);
WNDPROC fnpW;

#define PORT 7891
#define MESSAGE_NOTIFICATION 1337
unsigned clients;
CRITICAL_SECTION CritSection;

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

DWORD WINAPI handleClient(LPVOID param)
{
	SOCKET client = *(SOCKET*)param;
	cout << "Handling socket " << client << endl;
	fd_set read, write, err;
	
	FD_ZERO(&read); FD_ZERO(&write); FD_ZERO(&err);
	
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	int ret;

	char buf[100];

	while(1)
	{
		FD_SET(client, &read);
		FD_SET(client, &write);
		FD_SET(client, &err);
		ret = select(2, &read, &write, &err, &timeout);
		if (ret == 0) cout << "Timeout expired, client " << client << endl;
		if (ret < 0) break;
		if (ret)
		{
			//cout << "Total available sockets: " << ret << endl;
			if (FD_ISSET(client, &read))
			{
				memset(buf,0,sizeof(buf));
				if(recv(client, buf, sizeof(buf), 0) <=0) { cout << "Received 0 bytes from client " << client << ", terminating" << endl ; break; }
				cout << "Received from client: " << client << ", buffer: " << buf << endl;
				char *reverse = strrev(buf);
				send(client, reverse, strlen(reverse), 0);
			}

			if (FD_ISSET(client, &err))
			{
				cout << "Received error on client socket " << client << ", closing socket " << endl;
				break;
			}
			FD_CLR(client, &read); FD_CLR(client, &write); FD_CLR(client, &err);
		}

		Sleep(100);
	}

	EnterCriticalSection(&CritSection);
	clients--;
	LeaveCriticalSection(&CritSection);
	closesocket(client);
	return 0;
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

//	if((handle=CreateWindow("Button", "hidden", WS_MINIMIZE, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL)) == NULL)
//		cout << "Failed to create window " << endl;
  
//	cout << "Created window with handle " << handle << endl;

	//listen
	if(listen(s, 5))
		cout << "listen error " << endl;

	clients = 0;

	while(1)
	{
		struct sockaddr_in details_addr;
		int addrlen = sizeof(details_addr);

		cout << "Accepting " << endl;
		SOCKET client = accept(s,(sockaddr*)&details_addr, &addrlen); //blocking
		EnterCriticalSection(&CritSection);
		++clients;
		LeaveCriticalSection(&CritSection);

		cout << "Connected clients: " << clients << endl;

		cout << "Socket from client: " << client << endl;
		CreateThread(NULL,0,handleClient,&client,0,NULL);	
	}
}



int main()
{
	serve();
	closeConn();
	return 0;
}