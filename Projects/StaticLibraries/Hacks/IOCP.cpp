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

	bool IOCP::Init( short Port )
	{
		INFO_LOG << "IOCP Init() Started. " << VALUE( Port ) << EOL;
		if ( socket != INVALID_SOCKET )
		{
			WARNING_LOG << L"socket is valid already." << EOL;
			return false;
		}

		WSADATA wsaData;
		if ( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 )
		{
			ERROR_LOG << L"WSAStartup() failed." << EOL;
			return false;
		}
		iocpHandle = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );

		SYSTEM_INFO sysInfo;
		GetSystemInfo( &sysInfo );
		int threadsCount = sysInfo.dwNumberOfProcessors + 1/*AcceptThread*/;
		threads.reserve( threadsCount );
		for ( DWORD i = 0; i < sysInfo.dwNumberOfProcessors; ++i ) // need to make it set by config
		{
			threads.emplace_back( std::make_shared< std::thread >( &IOCP::IoThreadProcedure, this ) );
		}

		socket = WSASocket( AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED );
		if ( socket == INVALID_SOCKET )
		{
			ERROR_LOG << L"WSASocket() failed." << EOL;
			return false;
		}

		SOCKADDR_IN servAdr;
		memset( &servAdr, 0, sizeof( servAdr ) );
		servAdr.sin_family = AF_INET;
		servAdr.sin_addr.s_addr = htonl( INADDR_ANY );
		servAdr.sin_port = htons( Port );
		if ( bind( socket, reinterpret_cast<SOCKADDR*>(&servAdr), sizeof( servAdr ) ) == SOCKET_ERROR )
		{
			ERROR_LOG << L"bind() failed." << VALUE( Port ) << EOL;
			return false;
		}

		if ( listen( socket, SOMAXCONN ) == SOCKET_ERROR )
		{
			ERROR_LOG << L"listen() failed." << VALUE( Port ) << EOL;
			return false;
		}

		threads.emplace_back( std::make_shared< std::thread >( &IOCP::AcceptThreadProcedure, this ) );
		INFO_LOG << "IOCP Init() Finished. " << VALUE( threadsCount ) << EOL;

		return true;
	}

	void IOCP::Release()
	{
		INFO_LOG << L"IOCP Release() Started. " << VALUE( threads.size() ) << EOL;

		if ( socket != INVALID_SOCKET )
		{
			closesocket( socket );
			socket = INVALID_SOCKET;
		}

		if ( iocpHandle != INVALID_HANDLE_VALUE )
		{
			CloseHandle( iocpHandle );
			iocpHandle = INVALID_HANDLE_VALUE;
		}

		WSACleanup();

		for ( auto itr = threads.begin(); itr != threads.end(); ++itr )
		{
			(*itr)->join();
		}
		threads.clear();
		INFO_LOG << L"IOCP Release() Finished." << EOL;
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
			if ( socket == INVALID_SOCKET )
			{
				break;
			}

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
			if ( iocpHandle == INVALID_HANDLE_VALUE )
			{
				break;
			}

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