#pragma once

namespace Hacks
{
	class Logger
	{
	private:
		Logger() = default;
	public:
		~Logger() = default;
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
		std::wofstream file;
	};
}

#define INFO_LOG Hacks::Logger::GetInstance().Write( Hacks::Logger::Type::Info, std::move( std::wstringstream()
#define WARNING_LOG Hacks::Logger::GetInstance().Write( Hacks::Logger::Type::Warning, std::move( std::wstringstream()
#define ERROR_LOG Hacks::Logger::GetInstance().Write( Hacks::Logger::Type::Error, std::move( std::wstringstream()
#define EOL L" @" << __FUNCTION__ ) );