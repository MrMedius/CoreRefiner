#pragma once

#include "FocusManager.h"
#include "MouseUiInputAdapter.h"
#include "KeyboardUiInputAdapter.h"
#include "GamepadUiInputAdapter.h"

#include <cstddef>
#include <vector>

namespace Rgph
{
	class RenderGraph;
}

namespace Ui
{
	class UiButton;
	class IButtonView;

	enum class UiInputDominance
	{
		Mouse,
		NonPointer
	};

	struct UiButtonSlot
	{
		UiButton* button = nullptr;
		IButtonView* view = nullptr;
	};


	class UiRoot
	{
	public:
		UiRoot() = default;

		void Clear() noexcept;

		void AddButtonSlot(UiButton* btn, IButtonView* view);
		void RebuildTabOrderFromSlots();

		void InitLinkTechniques(Rgph::RenderGraph& rg);

		void UpdateAfterInput();
		void Submit(std::size_t channelMask) const;

		[[nodiscard]] FocusManager& Focus() noexcept { return focus_; }
		[[nodiscard]] const FocusManager& Focus() const noexcept { return focus_; }

		[[nodiscard]] UiInputDominance GetDominance() const noexcept { return dominance_; }


		void SetGamepadPlayerIndex(int idx) noexcept { gamepad_.SetPlayerIndex(idx); }

	private:

		static void StripPointerForWidgets_(UiInputFrame& out) noexcept;

		void ResetToMouseDominantState_();
		void ResetToNonPointerDominantState_();

		// Input dominance tracking
		FocusManager focus_{};
		MouseUiInputAdapter mouse_{};
		KeyboardUiInputAdapter keyboard_{};
		GamepadUiInputAdapter gamepad_{ 0 };

		std::vector<UiButtonSlot> slots_{};


		UiInputDominance dominance_{ UiInputDominance::Mouse };

		bool pointerPrimaryWasDown_{ false };

		bool hasLastPointer_{ false };
		float lastPointerLogicalX_{ 0.0f };
		float lastPointerLogicalY_{ 0.0f };
		static constexpr float kMouseMoveDominanceThresholdPx = 2.0f;
	};
}