#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <iterator>
#include <cstddef>

std::vector<std::string> TokenizeQuoted( const std::string& input );

std::wstring ToWide( const std::string& narrow );

std::wstring ToWideUtf8(std::string_view utf8);

std::string ToNarrow( const std::wstring& wide );

[[nodiscard]] std::string ToUtf8(std::wstring_view wide);

[[nodiscard]] std::size_t Utf8Next(std::string_view s, std::size_t byteIndex) noexcept;

[[nodiscard]] std::size_t Utf8Prev(std::string_view s, std::size_t byteIndex) noexcept;

[[nodiscard]] std::size_t Utf8CodepointCount(std::string_view s) noexcept;

// UTF-8 → UTF-16 code unit 数（Windows wchar / DirectWrite Span 下标）。只问长度，不分配。
[[nodiscard]] std::size_t Utf16CodeUnitCount(std::string_view utf8) noexcept;

template<class Iter>
void SplitStringIter( const std::string& s,const std::string& delim,Iter out )
{
	if( delim.empty() )
	{
		*out++ = s;
	}
	else
	{
		size_t a = 0,b = s.find( delim );
		for( ; b != std::string::npos;
			  a = b + delim.length(),b = s.find( delim,a ) )
		{
			*out++ = std::move( s.substr( a,b - a ) );
		}
		*out++ = std::move( s.substr( a,s.length() - a ) );
	}
}

std::vector<std::string> SplitString( const std::string& s,const std::string& delim );

bool StringContains( std::string_view haystack,std::string_view needle );