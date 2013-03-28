#include <winsock.h>
#include <iostream>

using namespace std;

SOCKET s;

bool connHost(int port, char* ip)
{
	WSADATA wsa;
	int error = WSAStartup(0x0202, &wsa);
	if (error) return false;
	if (wsa.wVersion != 0x0202) 
	{
		WSACleanup();
		return false;
	}
	sockaddr_in target;
	target.sin_family = AF_INET; //ipv4
	target.sin_port = port;
	target.sin_addr.s_addr = inet_addr(ip);
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		return false;
	}
	if (connect(s, (sockaddr*)&target, sizeof(target)) == SOCKET_ERROR)
	{
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

int main(int argc, char** argv)
{
	if(argc !=3) return -1;
	char *ip = argv[1];
	std::cout << "Ip: " << ip << std::endl;
	int port = atoi(argv[2]);
	int count = 0;
	std::cout << "Port " << port << std::endl;
	if(connHost(port, ip))
	{
		std::cout << "Connected X" << std::endl;
		char buf[100];
		memset(buf,0,100);

		while(count ++ < 10)
		{
			memset(buf,0,100);
			sprintf_s(buf, "I am %ul, cur_msg_count %d", GetCurrentProcessId(), count);
			cout << "Sending " << endl;
			send(s, buf, sizeof(buf), 0);
			Sleep(500);

			memset(buf, 0, sizeof(buf));
			cout << "Receiving " << endl;
			int received = recv(s,buf,sizeof(buf),0);

			cout << "REceived: <" << buf << ">, size " << received << endl;	
		}

		Sleep(10000);
	}
	else std::cout << "Not connected " << std::endl;
	closeConn();
	return 0;
}