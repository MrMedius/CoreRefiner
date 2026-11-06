#pragma once

#include "Util.h"

#include <cstddef>
#include <string>

namespace Ui
{
	// 多行 UTF-8 编辑缓冲与 caret（字节索引）。
	class UiUtf8EditBuffer
	{
	public:
		[[nodiscard]] const std::string& GetText() const noexcept { return text_; }
		[[nodiscard]] std::size_t GetCaretByteIndex() const noexcept { return caretByteIndex_; }

		void SetText(std::string utf8);
		void SetCaretByteIndex(std::size_t byteIndex) noexcept;

		void InsertUtf8(std::string_view utf8);
		void DeleteBackward();
		void DeleteForward();

		void MoveCaretLeft();
		void MoveCaretRight();
		void MoveCaretUp();
		void MoveCaretDown();
		void MoveCaretLineHome();
		void MoveCaretLineEnd();

		// 光标是否已在第一行。第一行再按上键就把焦点让出去。
		[[nodiscard]] bool IsCaretOnFirstLine() const noexcept;
		// 光标是否已在最后一行。最后一行再按下键就把焦点让出去。
		[[nodiscard]] bool IsCaretOnLastLine() const noexcept;

	private:
		[[nodiscard]] std::size_t FindLineStart(std::size_t byteIndex) const noexcept;
		[[nodiscard]] std::size_t FindLineEnd(std::size_t byteIndex) const noexcept;
		[[nodiscard]] std::size_t FindPrevLineStart(std::size_t byteIndex) const noexcept;
		[[nodiscard]] std::size_t FindNextLineStart(std::size_t byteIndex) const noexcept;
		[[nodiscard]] std::size_t ColumnOnLine(std::size_t lineStart, std::size_t byteIndex) const noexcept;
		[[nodiscard]] std::size_t OffsetFromColumn(std::size_t lineStart, std::size_t lineEnd, std::size_t column) const noexcept;

		void ClampCaret_() noexcept;

		std::string text_;
		std::size_t caretByteIndex_ = 0u;
	};
}
