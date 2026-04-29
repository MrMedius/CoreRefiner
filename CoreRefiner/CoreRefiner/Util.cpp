#include "Util.h"
#include <sstream>
#include <iomanip>
#include <Windows.h>

std::vector<std::string> TokenizeQuoted( const std::string& input )
{
	std::istringstream stream;
	stream.str( input );
	std::vector<std::string> tokens;
	std::string token;

	while( stream >> std::quoted( token ) )
	{
		tokens.push_back( std::move( token ) );
	}
	return tokens;
}

std::wstring ToWide( const std::string& narrow )
{
	wchar_t wide[512];
	mbstowcs_s( nullptr,wide,narrow.c_str(),_TRUNCATE );
	return wide;
}

std::wstring ToWideUtf8(std::string_view utf8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);
	if (len <= 0) return L"";
	std::wstring w(len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), w.data(), len);
	return w;
}

std::string ToNarrow( const std::wstring& wide )
{
	char narrow[512];
	wcstombs_s( nullptr,narrow,wide.c_str(),_TRUNCATE );
	return narrow;
}

std::vector<std::string> SplitString( const std::string& s,const std::string& delim )
{
	std::vector<std::string> strings;
	SplitStringIter( s,delim,std::back_inserter( strings ) );
	return strings;
}

bool StringContains( std::string_view haystack,std::string_view needle )
{
	return std::search( haystack.begin(),haystack.end(),needle.begin(),needle.end() ) != haystack.end();
}
