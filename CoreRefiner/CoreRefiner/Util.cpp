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

std::string ToUtf8(const std::wstring_view wide)
{
	if (wide.empty())
		return {};

	const int len = WideCharToMultiByte(
		CP_UTF8, 0,
		wide.data(), static_cast<int>(wide.size()),
		nullptr, 0, nullptr, nullptr);
	if (len <= 0)
		return {};

	std::string out(static_cast<std::size_t>(len), '\0');
	WideCharToMultiByte(
		CP_UTF8, 0,
		wide.data(), static_cast<int>(wide.size()),
		out.data(), len, nullptr, nullptr);
	return out;
}

std::size_t Utf8Next(const std::string_view s, const std::size_t byteIndex) noexcept
{
	if (byteIndex >= s.size())
		return s.size();

	const unsigned char lead = static_cast<unsigned char>(s[byteIndex]);
	std::size_t step = 1u;
	if ((lead & 0x80u) == 0u)
		step = 1u;
	else if ((lead & 0xE0u) == 0xC0u)
		step = 2u;
	else if ((lead & 0xF0u) == 0xE0u)
		step = 3u;
	else if ((lead & 0xF8u) == 0xF0u)
		step = 4u;

	const std::size_t next = byteIndex + step;
	return next > s.size() ? s.size() : next;
}

std::size_t Utf8Prev(const std::string_view s, const std::size_t byteIndex) noexcept
{
	if (byteIndex == 0u || s.empty())
		return 0u;

	std::size_t i = byteIndex;
	while (i > 0u)
	{
		--i;
		const unsigned char c = static_cast<unsigned char>(s[i]);
		if ((c & 0xC0u) != 0x80u)
			return i;
	}
	return 0u;
}

std::size_t Utf8CodepointCount(const std::string_view s) noexcept
{
	std::size_t count = 0u;
	for (std::size_t i = 0u; i < s.size(); i = Utf8Next(s, i))
		++count;
	return count;
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
