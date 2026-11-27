#include "ExceptionBase.h"
#include <sstream>


ExceptionBase::ExceptionBase( int line,const char* file ) noexcept
	:
	line( line ),
	file( file )
{}

const char* ExceptionBase::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* ExceptionBase::GetType() const noexcept
{
	return "Exception";
}

int ExceptionBase::GetLine() const noexcept
{
	return line;
}

const std::string& ExceptionBase::GetFile() const noexcept
{
	return file;
}

std::string ExceptionBase::GetOriginString() const noexcept
{
	std::ostringstream oss;
	oss << "[File] " << file << std::endl
		<< "[Line] " << line;
	return oss.str();
}