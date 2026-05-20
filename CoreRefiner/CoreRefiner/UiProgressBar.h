#pragma once

#include "IUiLogic.h"
#include "UiTypes.h"

namespace Ui
{
	class UiProgressBar : public IUiLogic
	{
	public:
		explicit UiProgressBar(UiRect bounds);

		void SetBounds(UiRect r) noexcept { bounds_ = r; }
		[[nodiscard]] const UiRect& GetBounds() const noexcept { return bounds_; }

		void SetRange(float minValue, float maxValue) noexcept;
		void SetValue(float value) noexcept;
		void SetIndeterminate(bool on) noexcept { indeterminate_ = on; }
		void SetEnabled(bool enabled) noexcept { enabled_ = enabled; }

		[[nodiscard]] float GetValue() const noexcept { return value_; }
		[[nodiscard]] float GetNormalized() const noexcept;
		[[nodiscard]] bool IsIndeterminate() const noexcept { return indeterminate_; }
		[[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

		void Update(const UiInputFrame& frame, const FocusManager& focus) override;
		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept override { return kInvalidFocusHandle; }
		void ResetPointerInteraction() noexcept override {}
		[[nodiscard]] bool IsFocusable() const noexcept override { return false; }

	private:
		UiRect bounds_{};
		float min_ = 0.0f;
		float max_ = 1.0f;
		float value_ = 0.0f;
		bool indeterminate_ = false;
		bool enabled_ = true;
	};
}
