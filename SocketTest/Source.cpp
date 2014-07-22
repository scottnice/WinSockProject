#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <mutex>
#include <vector>
// Need to link with Ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

namespace StringUtilities
{
	using namespace std;
	string intToString(int i)
	{
		char buf[10];
		sprintf_s(buf, 10, "%d", i);
		return string(buf);
	}
}

namespace WinSockLibrary
{
	using std::string;
	using std::mutex;
	using std::exception;
	using std::lock_guard;
	class StandardException : std::exception
	{
	private:
		std::string message;
	public:
		StandardException(std::string m) : message(m){}
		virtual ~StandardException() throw(){}
		virtual const char* what() throw() { return message.c_str(); }
	};

	class Socket
	{
		friend class Connection;
	private:
		SOCKET ConnectSocket;

	public:

		Socket()
		{
			ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (ConnectSocket == INVALID_SOCKET) {
				throw StandardException(string("socket function failed with error: %ld\n") + StringUtilities::intToString(WSAGetLastError()));
			}
		}
	};

	

	class Connection
	{
	private:
		static unsigned int refCount;
		static mutex m_mutex;
		sockaddr_in clientService;
		WSADATA wsaData;
		Socket ConnectSocket;

	public:
		Connection(unsigned short port, const char* addr)
		{
			m_mutex.lock();
			if (refCount == 0)
			{
				refCount++;
				int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
				if (iResult != NO_ERROR) {
					throw StandardException(string("WSAStartup function failed with error: %d\n") + StringUtilities::intToString(iResult));
				}
			}
			else
			{
				refCount++;
			}

			clientService.sin_family = AF_INET;
			clientService.sin_addr.s_addr = inet_addr(addr);
			clientService.sin_port = htons(port);

		}
		Connection(unsigned short port, string addr) : Connection(port, addr) {}

		~Connection()
		{
			lock_guard<mutex> lock(m_mutex);
			if (refCount == 1)
			{
				WSACleanup();
			}	
		}

		bool bind()
		{
			int iResult;
			iResult = ::bind(ConnectSocket.ConnectSocket, (sockaddr*)&clientService, sizeof(clientService));
			if (iResult == SOCKET_ERROR) {
				wprintf(L"Bind function failed with error: %ld\n", WSAGetLastError());
				iResult = closesocket(ConnectSocket.ConnectSocket);
				if (iResult == SOCKET_ERROR)
					wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
				return false;
			}
		}

	};
}

namespace WinSock
{
	bool tryConnect(u_short port)
	{
		//----------------------
		// Initialize Winsock
		WSADATA wsaData;
		int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != NO_ERROR) {
			wprintf(L"WSAStartup function failed with error: %d\n", iResult);
			return 1;
		}
		//----------------------
		// Create a SOCKET for connecting to server
		SOCKET ConnectSocket;
		ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (ConnectSocket == INVALID_SOCKET) {
			wprintf(L"socket function failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		int opt = 1;
		setsockopt(ConnectSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
		if (iResult == SOCKET_ERROR) {
			wprintf(L"set socket option error function failed with error: %ld\n", WSAGetLastError());
			iResult = closesocket(ConnectSocket);
			if (iResult == SOCKET_ERROR)
				wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}
		//----------------------
		// The sockaddr_in structure specifies the address family,
		// IP address, and port of the server to be connected to.
		sockaddr_in clientService;
		clientService.sin_family = AF_INET;
		clientService.sin_addr.s_addr = inet_addr("127.0.0.1");
		clientService.sin_port = htons(port);


		iResult = bind(ConnectSocket, (sockaddr*)&clientService, sizeof(clientService));
		if (iResult == SOCKET_ERROR) {
			wprintf(L"Bind function failed with error: %ld\n", WSAGetLastError());
			iResult = closesocket(ConnectSocket);
			if (iResult == SOCKET_ERROR)
				wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		//----------------------
		// Connect to server.
		iResult = connect(ConnectSocket, (SOCKADDR *)& clientService, sizeof (clientService));
		if (iResult == SOCKET_ERROR) {
			wprintf(L"connect function failed with error: %ld\n", WSAGetLastError());
			iResult = closesocket(ConnectSocket);
			if (iResult == SOCKET_ERROR)
				wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		wprintf(L"Connected to server.\n");
		char buf[500];
		char sendbuf[] = "123456789 \0";


		iResult = send(ConnectSocket, sendbuf, strlen(sendbuf)+1, 0);
		if (iResult == SOCKET_ERROR) {
			wprintf(L"send failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}

		

		do{
			iResult = recv(ConnectSocket, buf, 500, 0);
			if (iResult > 0)
				wprintf(L"Bytes received: %d\n", iResult);
			else if (iResult == 0)
				wprintf(L"Connection closed\n");
			else if (iResult < 0)
				wprintf(L"recv failed with error: %d\n", WSAGetLastError());

		} while (iResult != strlen(sendbuf)+1);
		
		int size = strlen(buf);
		std::string s(buf, size);
		printf(s.c_str());

		iResult = shutdown(ConnectSocket, SD_BOTH);
		if (iResult == SOCKET_ERROR) {
			wprintf(L"Shutdown function failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}


		iResult = closesocket(ConnectSocket);
		if (iResult == SOCKET_ERROR) {
			wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		printf("SUCCESS!");
		WSACleanup();
		return 0;
	}
}


int wmain()
{
	WinSock::tryConnect(20001);

	system("PAUSE");
}