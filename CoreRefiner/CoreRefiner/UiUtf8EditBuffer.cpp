#include "UiUtf8EditBuffer.h"

#include <algorithm>

namespace Ui
{
	void UiUtf8EditBuffer::SetText(std::string utf8)
	{
		text_ = std::move(utf8);
		ClampCaret_();
	}

	void UiUtf8EditBuffer::SetCaretByteIndex(const std::size_t byteIndex) noexcept
	{
		caretByteIndex_ = byteIndex;
		ClampCaret_();
	}

	void UiUtf8EditBuffer::InsertUtf8(const std::string_view utf8)
	{
		if (utf8.empty())
			return;

		text_.insert(caretByteIndex_, utf8);
		caretByteIndex_ += utf8.size();
		ClampCaret_();
	}

	void UiUtf8EditBuffer::DeleteBackward()
	{
		if (caretByteIndex_ == 0u)
			return;

		const std::size_t prev = Utf8Prev(text_, caretByteIndex_);
		text_.erase(prev, caretByteIndex_ - prev);
		caretByteIndex_ = prev;
	}

	void UiUtf8EditBuffer::DeleteForward()
	{
		if (caretByteIndex_ >= text_.size())
			return;

		const std::size_t next = Utf8Next(text_, caretByteIndex_);
		text_.erase(caretByteIndex_, next - caretByteIndex_);
	}

	void UiUtf8EditBuffer::MoveCaretLeft()
	{
		caretByteIndex_ = Utf8Prev(text_, caretByteIndex_);
	}

	void UiUtf8EditBuffer::MoveCaretRight()
	{
		caretByteIndex_ = Utf8Next(text_, caretByteIndex_);
	}

	void UiUtf8EditBuffer::MoveCaretUp()
	{
		const std::size_t lineStart = FindLineStart(caretByteIndex_);
		if (lineStart == 0u && caretByteIndex_ == 0u)
			return;

		const std::size_t prevLineStart = FindPrevLineStart(caretByteIndex_);
		const std::size_t prevLineEnd = FindLineEnd(prevLineStart);
		const std::size_t column = ColumnOnLine(lineStart, caretByteIndex_);
		caretByteIndex_ = OffsetFromColumn(prevLineStart, prevLineEnd, column);
	}

	void UiUtf8EditBuffer::MoveCaretDown()
	{
		const std::size_t lineStart = FindLineStart(caretByteIndex_);
		const std::size_t lineEnd = FindLineEnd(lineStart);
		if (lineEnd >= text_.size())
			return;

		const std::size_t nextLineStart = lineEnd + 1u;
		const std::size_t nextLineEnd = FindLineEnd(nextLineStart);
		const std::size_t column = ColumnOnLine(lineStart, caretByteIndex_);
		caretByteIndex_ = OffsetFromColumn(nextLineStart, nextLineEnd, column);
	}

	void UiUtf8EditBuffer::MoveCaretLineHome()
	{
		caretByteIndex_ = FindLineStart(caretByteIndex_);
	}

	void UiUtf8EditBuffer::MoveCaretLineEnd()
	{
		caretByteIndex_ = FindLineEnd(FindLineStart(caretByteIndex_));
	}

	std::size_t UiUtf8EditBuffer::FindLineStart(const std::size_t byteIndex) const noexcept
	{
		const std::size_t clamped = std::min(byteIndex, text_.size());
		const std::size_t pos = text_.rfind('\n', clamped == 0u ? 0u : clamped - 1u);
		return pos == std::string::npos ? 0u : pos + 1u;
	}

	std::size_t UiUtf8EditBuffer::FindLineEnd(const std::size_t byteIndex) const noexcept
	{
		const std::size_t pos = text_.find('\n', byteIndex);
		return pos == std::string::npos ? text_.size() : pos;
	}

	std::size_t UiUtf8EditBuffer::FindPrevLineStart(const std::size_t byteIndex) const noexcept
	{
		const std::size_t lineStart = FindLineStart(byteIndex);
		if (lineStart == 0u)
			return 0u;

		return FindLineStart(lineStart - 1u);
	}

	std::size_t UiUtf8EditBuffer::FindNextLineStart(const std::size_t byteIndex) const noexcept
	{
		const std::size_t lineEnd = FindLineEnd(byteIndex);
		if (lineEnd >= text_.size())
			return lineEnd;

		return lineEnd + 1u;
	}

	std::size_t UiUtf8EditBuffer::ColumnOnLine(
		const std::size_t lineStart,
		const std::size_t byteIndex) const noexcept
	{
		return Utf8CodepointCount(std::string_view(text_).substr(lineStart, byteIndex - lineStart));
	}

	std::size_t UiUtf8EditBuffer::OffsetFromColumn(
		const std::size_t lineStart,
		const std::size_t lineEnd,
		const std::size_t column) const noexcept
	{
		std::size_t index = lineStart;
		std::size_t count = 0u;
		while (index < lineEnd && count < column)
		{
			index = Utf8Next(text_, index);
			++count;
		}
		return index;
	}

	void UiUtf8EditBuffer::ClampCaret_() noexcept
	{
		if (caretByteIndex_ > text_.size())
			caretByteIndex_ = text_.size();
	}
}
