#pragma once

class Logger
{
private:
	Logger();
public:
	~Logger();
	static Logger& GetInstance();

public:
	void Init( std::wstring filePath );

	enum Type
	{
		Info,
		Warning,
		Error,
	};
	static std::wstring ToString( Type type );

	void Write( Type type, const std::wostream&& stream );

private:
	CRITICAL_SECTION listenersCs;
	std::wofstream file;
};

#define INFO_LOG Logger::GetInstance().Write( Logger::Type::Info, std::move( std::wstringstream()
#define WARNING_LOG Logger::GetInstance().Write( Logger::Type::Warning, std::move( std::wstringstream()
#define ERROR_LOG Logger::GetInstance().Write( Logger::Type::Error, std::move( std::wstringstream()
#define EOL L" @" << __FUNCTION__ ) );