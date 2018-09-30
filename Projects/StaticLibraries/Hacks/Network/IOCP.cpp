#include "stdafx.h"
#include "IOCP.h"
#include "../Logger.h"

namespace Hacks
{
	namespace Network
	{
		Connection::Connection()
			: socket( INVALID_SOCKET )
		{
		}

		bool Connection::Init( SOCKET newSocket, const sockaddr_in& addr )
		{
			if ( newSocket == INVALID_SOCKET )
			{
				return false;
			}
			socket = newSocket;

			// is using INET_ADDRSTRLEN fine with IPV4 and IPV6 compatibility..? using sockeAddr.sin_family too.
			if ( InetNtopW( addr.sin_family, &addr.sin_addr, ipAddress, sizeof( ipAddress ) ) == nullptr )
			{
				ERROR_LOG << L"InetNtopW() failed. " << VALUE( WSAGetLastError() ) << EOL;
				return false;
			}

			return true;
		}

		SOCKET Connection::GetSocket() const
		{
			return socket;
		}

		const wchar_t* Connection::GetIpAddress() const
		{
			return ipAddress;
		}

		Connection::~Connection()
		{
			if ( socket != INVALID_SOCKET )
			{
				closesocket( socket );
			}
		}

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
			int result = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
			if ( result != 0 )
			{
				ERROR_LOG << L"WSAStartup() failed. " << VALUE( result ) << EOL;
				return false;
			}

			socket = WSASocket( AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED );
			if ( socket == INVALID_SOCKET )
			{
				ERROR_LOG << L"WSASocket() failed. " << VALUE( WSAGetLastError() ) << EOL;
				return false;
			}

			SOCKADDR_IN servAdr;
			memset( &servAdr, 0, sizeof( servAdr ) );
			servAdr.sin_family = AF_INET;
			servAdr.sin_addr.s_addr = htonl( INADDR_ANY );
			servAdr.sin_port = htons( Port );
			if ( bind( socket, reinterpret_cast<SOCKADDR*>( &servAdr ), sizeof( servAdr ) ) == SOCKET_ERROR )
			{
				ERROR_LOG << L"bind() failed." << VALUE( WSAGetLastError() ) << VALUE( Port ) << EOL;
				return false;
			}

			if ( listen( socket, SOMAXCONN ) == SOCKET_ERROR )
			{
				ERROR_LOG << L"listen() failed." << VALUE( WSAGetLastError() ) << EOL;
				return false;
			}

			iocpHandle = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
			if ( iocpHandle == NULL )
			{
				ERROR_LOG << L"CreateIoCompletionPort() failed." << VALUE( WSAGetLastError() ) << EOL;
				return false;
			}

			SYSTEM_INFO sysInfo;
			GetSystemInfo( &sysInfo );
			int threadsCount = max( 1, sysInfo.dwNumberOfProcessors ) + 1/*AcceptThread*/;
			threads.reserve( threadsCount );
			for ( DWORD i = 0; i < sysInfo.dwNumberOfProcessors; ++i ) // need to make it can be set by config
			{
				threads.emplace_back( std::make_shared< std::thread >( &IOCP::IoThreadProcedure, this ) );
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
				if ( closesocket( socket ) == SOCKET_ERROR )
				{
					ERROR_LOG << L"closesocket() failed. " << VALUE( WSAGetLastError() ) << EOL;
				}
				socket = INVALID_SOCKET;
			}

			if ( iocpHandle != INVALID_HANDLE_VALUE )
			{
				if ( CloseHandle( iocpHandle ) == 0 )
				{
					ERROR_LOG << L"CloseHandle() failed. " << VALUE( GetLastError() ) << EOL;
				}
				iocpHandle = INVALID_HANDLE_VALUE;
			}

			if ( WSACleanup() == SOCKET_ERROR )
			{
				ERROR_LOG << L"WSACleanup() failed. " << VALUE( WSAGetLastError() ) << EOL;
			}

			for ( auto itr = threads.begin(); itr != threads.end(); ++itr )
			{
				( *itr )->join();
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

		void IOCP::AcceptThreadProcedure()
		{
			while ( 1 )
			{
				sockaddr_in sockeAddr = {};
				int socketAddressSize = sizeof( sockeAddr );
				SOCKET clientSocket = accept( socket, reinterpret_cast<sockaddr*>( &sockeAddr ), &socketAddressSize );
				if ( clientSocket == INVALID_SOCKET )
				{
					if ( socket == INVALID_SOCKET )
					{
						break;
					}
					ERROR_LOG << L"accept() failed. " << VALUE( WSAGetLastError() ) << EOL;
					continue;
				}

				// need to pool connections, buffers
				Connection* connection = new Connection();
				if ( connection->Init( clientSocket, sockeAddr ) == false )
				{
					ERROR_LOG << L"connection->Init() failed. " << EOL;
					continue;
				}
				INFO_LOG << L"A connection has been established. " << VALUE( connection->GetIpAddress() ) << VALUE( sockeAddr.sin_family ) << EOL;

				if ( CreateIoCompletionPort( reinterpret_cast<HANDLE>( connection->GetSocket() ), iocpHandle, reinterpret_cast<ULONG_PTR>( connection ), 0 ) == NULL )
				{
					ERROR_LOG << L"CreateIoCompletionPort() failed." << VALUE( WSAGetLastError() ) << VALUE( connection->GetIpAddress() ) << EOL;
					delete connection;
					continue;
				}

				IocpBuffer* buffer = new IocpBuffer();
				buffer->usage = IocpBuffer::READ;
				DWORD flag = 0;
				if ( WSARecv( connection->GetSocket(), &buffer->wsaBuf, 1, nullptr, &flag, buffer, nullptr ) == SOCKET_ERROR )
				{
					int error = WSAGetLastError();
					if ( error != WSA_IO_PENDING )
					{
						ERROR_LOG << L"WSARecv() failed. " << VALUE( error ) << VALUE( connection->GetIpAddress() ) << EOL;
						delete connection;
						delete buffer;
						continue;
					}
				}
			}
		}


		void IOCP::IoThreadProcedure()
		{
			while ( 1 )
			{
				Connection* connection = nullptr;
				IocpBuffer* buffer = nullptr;
				DWORD transferredBytes = 0;
				// how to relase sockets and buffers on heap?
				if ( GetQueuedCompletionStatus( iocpHandle, &transferredBytes, reinterpret_cast<PULONG_PTR>( &connection ), reinterpret_cast<LPOVERLAPPED*>( &buffer ), INFINITE ) == 0 )
				{
					int error = GetLastError();
					if ( iocpHandle == INVALID_HANDLE_VALUE && error == ERROR_ABANDONED_WAIT_0 )
					{
						break;
					}

					ERROR_LOG << L"GetQueuedCompletionStatus() failed." << VALUE( error ) << EOL;
					continue;
				}

				if ( transferredBytes == 0 )
				{
					ERROR_LOG << L"A connection has been disconnected. " << VALUE( connection->GetIpAddress() ) << EOL;
					delete connection;
					delete buffer;
					continue;
				}

				if ( buffer == nullptr )
				{
					ERROR_LOG << L"buffer is nullptr. " << VALUE( connection->GetIpAddress() ) << EOL;
					continue;
				}

				switch ( buffer->usage )
				{
				case IocpBuffer::Usage::READ:
					{
						Send( *connection, buffer->wsaBuf.buf, transferredBytes );
						DWORD flag = 0;
						WSARecv( connection->GetSocket(), &buffer->wsaBuf, 1, nullptr, &flag, buffer, nullptr );
					}
					break;
				case IocpBuffer::Usage::WRITE:
					delete buffer;
					break;
				default:
					delete buffer;
					ERROR_LOG << L"Invalid usage. " << VALUE( buffer->usage ) << EOL;
					break;
				}
			}
		}

		void IOCP::Send( const Connection& connection, const char* buffer, int len )
		{
			WSABUF wsaBuf;
			wsaBuf.len = len;
			wsaBuf.buf = const_cast<char*>( buffer );
			DWORD tempLen = len;
			WSASend( connection.GetSocket(), &wsaBuf, 1, &tempLen, 0, nullptr, NULL );
		}
	}
}