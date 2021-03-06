// DevTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <Ws2tcpip.h>
#include "Hacks/Network/IOCP.h"
#include "Hacks/Logger.h"

int main()
{
	Hacks::Logger::GetInstance().Init( L"Log.log" );

	Hacks::Network::IOCP iocp;
	if ( iocp.Init( 12345 ) == false )
	{
		iocp.Release();
		return 0;
	}

	SOCKET cliSocket;
	WSADATA wsaData;
	SOCKADDR_IN servAdr;
	if ( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 )
	{
		return 0;
	}

	cliSocket = socket( PF_INET, SOCK_STREAM, 0 );

	if ( cliSocket == INVALID_SOCKET )
	{
		return 0;
	}

	memset( &servAdr, 0, sizeof( servAdr ) );
	servAdr.sin_family = AF_INET;
	InetPton( AF_INET, TEXT( "127.0.0.1" ), &servAdr.sin_addr.s_addr );
	servAdr.sin_port = htons( 12345 );

	if ( connect( cliSocket, (SOCKADDR*)&servAdr, sizeof( servAdr ) ) == SOCKET_ERROR )
	{
		return 0;
	}

	//unsigned long arg = 1;
	//ioctlsocket( socket, FIONBIO, &arg );

	while ( 1 )
	{
		std::string abc;
		std::cin >> abc;
		send( cliSocket, abc.c_str(), abc.length(), 0 );

		char buf[ 1024 ] = {};
		recv(cliSocket, buf, 1023, 0);
		std::cout << buf << std::endl;
	}

	iocp.Release();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
