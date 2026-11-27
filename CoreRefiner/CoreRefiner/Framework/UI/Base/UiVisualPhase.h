#pragma once

namespace Ui
{
	enum class UiVisualPhase
	{
		Normal,
		Focused,
		Pressed,
		Disabled
	};

	
	// Unified visual stage priority：Disabled → Pressed → Focused → Normal。
	// enabled		Check Component is enabled
	// pressVisual	Check Press state
	// pointerOver  Check Focused state
	// focusManagerFocused Check FocusManager has hovered
	
	[[nodiscard]] inline UiVisualPhase ComputeStandardPhase(
		const bool enabled,
		const bool pressVisual,
		const bool pointerOver,
		const bool focusManagerFocused) noexcept
	{
		if (!enabled)
			return UiVisualPhase::Disabled;
		if (pressVisual)
			return UiVisualPhase::Pressed;
		if (pointerOver || focusManagerFocused)
			return UiVisualPhase::Focused;
		return UiVisualPhase::Normal;
	}
}
