#pragma once

#include "FocusManager.h"

#include "IUiComponent.h"

#include "IUiPopupConsumer.h"

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

	enum class UiInputDominance

	{

		Mouse,

		NonPointer

	};



	class UiRoot

	{

	public:

		UiRoot() = default;



		void Clear() noexcept;



		void AddUiComponent(IUiComponent* component);

		void RebuildTabOrder();



		void InitLinkTechniques(Rgph::RenderGraph& rg);



		void UpdateAfterInput();

		void Submit(std::size_t channelMask) const;



		[[nodiscard]] FocusManager& Focus() noexcept { return focus_; }

		[[nodiscard]] const FocusManager& Focus() const noexcept { return focus_; }



		[[nodiscard]] FocusHandle GetFocus() const noexcept { return focus_.Focused(); }



		[[nodiscard]] UiInputDominance GetDominance() const noexcept { return dominance_; }



		void SetGamepadPlayerIndex(int idx) noexcept { gamepad_.SetPlayerIndex(idx); }



	private:

		static void StripPointerForWidgets_(UiInputFrame& out) noexcept;

		static void StripPrimaryPointer_(UiInputFrame& out) noexcept;



		void ResetToMouseDominantState_();

		void ResetToNonPointerDominantState_();



		[[nodiscard]] IUiPopupConsumer* FindOpenPopup_() const noexcept;



		[[nodiscard]] IUiComponent* PopupAsComponent_(IUiPopupConsumer* popup) const noexcept;



		FocusManager focus_{};

		MouseUiInputAdapter mouse_{};

		KeyboardUiInputAdapter keyboard_{};

		GamepadUiInputAdapter gamepad_{ 0 };



		std::vector<IUiComponent*> components_{};



		UiInputDominance dominance_{ UiInputDominance::Mouse };



		bool pointerPrimaryWasDown_{ false };



		bool hasLastPointer_{ false };

		float lastPointerLogicalX_{ 0.0f };

		float lastPointerLogicalY_{ 0.0f };

		static constexpr float kMouseMoveDominanceThresholdPx = 2.0f;

	};

}

