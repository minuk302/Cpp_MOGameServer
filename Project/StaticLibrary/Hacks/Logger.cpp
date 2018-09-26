#include "stdafx.h"
#include "Logger.h"

Logger::Logger()
{
	::InitializeCriticalSection( &listenersCs );
}

Logger::~Logger()
{
	::DeleteCriticalSection( &listenersCs );
}

Logger& Logger::GetInstance()
{
	static Logger logger;
	return logger;
}

void Logger::Init( std::wstring filePath )
{
	if ( file.is_open() == true )
	{
		file << L"Changed logging file to " << filePath << std::endl;
		file.close();
	}

	file.open( filePath );
}

std::wstring Logger::ToString( Type type )
{
	switch ( type )
	{
	case Type::Info:
		return L"[Info] ";
	case Type::Warning:
		return L"[Warning] ";
	case Type::Error:
		return L"[Error] ";
	default:
		break;
	}

	return L"[Unknown:" + std::to_wstring( type ) + L"]";
}

void Logger::Write( Type type, const std::wostream&& stream )
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	time_t rawTime = std::chrono::system_clock::to_time_t( now );
	tm localTime;
	localtime_s( &localTime, &rawTime );

	wchar_t header[ 64/*HeaderLength*/ ] = {};
	wsprintf( header, L"[%04u-%02u-%02u %02u:%02u:%02u.%03u]%s ", localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday, localTime.tm_hour, localTime.tm_min, localTime.tm_sec, std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000, ToString( type ).c_str() );

	std::wostringstream logStream;
	logStream << header << stream.rdbuf() << std::endl;

	if ( file.is_open() == true )
	{
		file << logStream.str();
	}

#ifdef _DEBUG
	OutputDebugString( logStream.str().c_str() );
#endif
}