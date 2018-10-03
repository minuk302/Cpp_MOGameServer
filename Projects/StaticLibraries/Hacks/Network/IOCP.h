#pragma once

namespace Hacks
{
	namespace Network
	{
		class Connection
		{
		public:
			Connection();
			~Connection();

		public:
			bool Init( SOCKET newSocket, const sockaddr_in& addr );

			SOCKET GetSocket() const;
			const wchar_t* GetIpAddress() const;

		private:
			SOCKET socket;
			wchar_t ipAddress[ INET_ADDRSTRLEN ];
		};
		struct IocpBuffer;

		class IOCP
		{
		public:
			IOCP();
			~IOCP();

			bool Init( short Port );
			void Release();

		private:
			void AcceptThreadProcedure();
			void IoThreadProcedure();
			void Send( const Connection& connection, const char* buffer, int len );

		private:
			SOCKET socket;
			HANDLE iocpHandle;
			std::vector< std::shared_ptr< std::thread > > threads;
		};
	}
}