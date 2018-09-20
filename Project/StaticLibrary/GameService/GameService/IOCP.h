#pragma once

class IOCP
{
public:
	IOCP();
	~IOCP();

	void Init( int Port );
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