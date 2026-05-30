#pragma once

#include "UiTextField.h"
#include "UiVisualPhase.h"

#include <string>

namespace Ui
{
	struct TextFieldViewModel
	{
		UiVisualPhase phase = UiVisualPhase::Normal;
		std::string textUtf8;
		std::string placeholderUtf8;
		std::string imeCompositionUtf8;
		bool imeCompositionActive = false;
		std::size_t caretByteIndex = 0u;
		bool showCaret = false;
		float fontSize = 18.0f;
	};

	inline TextFieldViewModel MakeTextFieldViewModel(const UiTextField& field)
	{
		return TextFieldViewModel{
			.phase = field.GetVisualPhase(),
			.textUtf8 = field.GetText(),
			.placeholderUtf8 = field.GetPlaceholder(),
			.imeCompositionUtf8 = field.GetImeComposition(),
			.imeCompositionActive = field.ImeCompositionActive(),
			.caretByteIndex = field.GetCaretByteIndex(),
			.showCaret = field.ShowCaret() && field.GetVisualPhase() == UiVisualPhase::Focused,
			.fontSize = field.GetFontSize()
		};
	}
}
