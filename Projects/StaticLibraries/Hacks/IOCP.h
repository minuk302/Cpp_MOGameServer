#pragma once

namespace Hacks
{
	class IOCP
	{
	public:
		IOCP();
		~IOCP();

		void Init( short Port );
		void Release();

	private:
		void AcceptThreadProcedure();
		void IoThreadProcedure();
		void Send( SOCKET socket, const char* buffer, int len );

	private:
		SOCKET socket;
		HANDLE iocpHandle;
		std::vector< std::shared_ptr< std::thread > > threads;
	};
}