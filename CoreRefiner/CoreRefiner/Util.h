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

/** @brief UTF-16 宽字符串转 UTF-8（与 ToWideUtf8 成对）。 */
[[nodiscard]] std::string ToUtf8(std::wstring_view wide);

/** @brief UTF-8 中下一个 codepoint 的字节偏移；若已在末尾则返回 size。 */
[[nodiscard]] std::size_t Utf8Next(std::string_view s, std::size_t byteIndex) noexcept;

/** @brief UTF-8 中上一个 codepoint 的字节偏移；若在开头则返回 0。 */
[[nodiscard]] std::size_t Utf8Prev(std::string_view s, std::size_t byteIndex) noexcept;

/** @brief 统计 UTF-8 字符串中的 codepoint 数量。 */
[[nodiscard]] std::size_t Utf8CodepointCount(std::string_view s) noexcept;

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