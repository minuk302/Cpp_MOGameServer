#include "stdafx.h"
#include "IOCP.h"
#include "Logger.h"

namespace Hacks
{

	IOCP::IOCP()
		: socket( INVALID_SOCKET )
		, iocpHandle( INVALID_HANDLE_VALUE )
	{
	}

	IOCP::~IOCP()
	{
	}

	void IOCP::Init( short Port )
	{
		// need more logs to print errors and system values
		if ( socket != INVALID_SOCKET )
		{
			// already running
			return;
		}

		WSADATA wsaData;
		if ( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 )
		{
			return;
		}
		iocpHandle = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );

		SYSTEM_INFO sysInfo;
		GetSystemInfo( &sysInfo );
		threads.reserve( sysInfo.dwNumberOfProcessors + 1/*AcceptThread*/ );
		for ( DWORD i = 0; i < sysInfo.dwNumberOfProcessors; ++i ) // need to make it set by config
		{
			threads.emplace_back( std::make_shared< std::thread >( &IOCP::IoThreadProcedure, *this ) );
		}

		socket = WSASocket( AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED );
		if ( socket == INVALID_SOCKET )
		{
			return;
		}

		SOCKADDR_IN servAdr;
		memset( &servAdr, 0, sizeof( servAdr ) );
		servAdr.sin_family = AF_INET;
		servAdr.sin_addr.s_addr = htonl( INADDR_ANY );
		servAdr.sin_port = htons( Port );
		if ( bind( socket, reinterpret_cast<SOCKADDR*>(&servAdr), sizeof( servAdr ) ) == SOCKET_ERROR )
		{
			return;
		}

		if ( listen( socket, SOMAXCONN ) == SOCKET_ERROR )
		{
			return;
		}

		threads.emplace_back( std::make_shared< std::thread >( &IOCP::AcceptThreadProcedure, *this ) );
	}

	void IOCP::Release()
	{
		// close all sockets(clients too), make threads to return if server socket is invalid

		closesocket( socket );
		socket = INVALID_SOCKET;

		CloseHandle( iocpHandle );
		iocpHandle = INVALID_HANDLE_VALUE;

		WSACleanup();

		for ( auto itr = threads.begin(); itr != threads.end(); ++itr )
		{
			(*itr)->join();
		}
		threads.clear();
	}

#define BUF_SIZE 1024
	struct IocpBuffer : public OVERLAPPED // temporary
	{
		enum Usage
		{
			READ,
			WRITE,
			NONE
		};

		IocpBuffer()
			: OVERLAPPED()
			, usage( Usage::NONE )
		{
			wsaBuf.buf = buffer;
			wsaBuf.len = BUF_SIZE;
		}

		WSABUF wsaBuf;
		char buffer[ BUF_SIZE ];
		Usage usage;
	};

	struct Connection
	{
		Connection( SOCKET socket_ )
			: socket( socket_ )
		{
		}

		SOCKET socket;
	};

	void IOCP::AcceptThreadProcedure()
	{
		while ( 1 )
		{
			SOCKADDR_IN clientAddressInfo;
			int addressInfoLength = sizeof( clientAddressInfo );
			SOCKET clientSocket = accept( socket, (SOCKADDR*)&clientAddressInfo, &addressInfoLength );
			if ( clientSocket == INVALID_SOCKET )
			{
				continue;
			}

			// better to pool connections
			Connection* connection = new Connection( clientSocket );
			CreateIoCompletionPort( (HANDLE)clientSocket, iocpHandle, reinterpret_cast<ULONG_PTR>(connection), 0 );

			IocpBuffer* buffer = new IocpBuffer();
			buffer->usage = IocpBuffer::Usage::READ;
			WSARecv( clientSocket, &buffer->wsaBuf, 1, nullptr, nullptr, buffer, nullptr );
		}
	}


	void IOCP::IoThreadProcedure()
	{
		while ( 1 )
		{
			Connection* connection = nullptr;
			IocpBuffer* buffer = nullptr;
			GetQueuedCompletionStatus( iocpHandle, nullptr, reinterpret_cast<PULONG_PTR>(&connection), reinterpret_cast<LPOVERLAPPED*>(&buffer), INFINITE );

			if ( buffer == nullptr )
			{
				continue;
			}
			switch ( buffer->usage )
			{
			case IocpBuffer::Usage::READ:
				if ( buffer->InternalHigh == 0 )
				{
					closesocket( connection->socket );
					delete connection;
					delete buffer;
					break;
				}
				WSARecv( connection->socket, &buffer->wsaBuf, 1, nullptr, nullptr, buffer, nullptr );
				break;
			case IocpBuffer::Usage::WRITE:
			case IocpBuffer::Usage::NONE: // needs to print logs
				delete buffer;
				break;
			}
		}
	}

	void IOCP::Send( SOCKET sender, const char* buffer, int len )
	{
		WSABUF wsaBuf;
		wsaBuf.len = len;
		wsaBuf.buf = const_cast<char*>(buffer);
		WSASend( sender, &(wsaBuf), 1, NULL, 0, nullptr, NULL );
	}
}