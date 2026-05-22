#pragma once
#include "FocusTypes.h"
#include "IUiComponent.h"
#include "ToggleCanvasView.h"
#include "UiRoot.h"
#include "UiToggle.h"
#include "UiTypes.h"

#include <DirectXMath.h>
#include <memory>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

namespace Ui
{
	class ToggleCanvasComponent final : public IUiComponent
	{
	public:
		ToggleCanvasComponent(
			Graphics& gfx,
			FocusHandle focusHandle,
			unsigned pixelSize,
			ToggleCanvasStyle style = {});

		ToggleCanvasComponent(
			Graphics& gfx,
			FocusHandle focusHandle,
			float centerX,
			float centerY,
			float size,
			ToggleCanvasStyle style = {});

		ToggleCanvasComponent(const ToggleCanvasComponent&) = delete;
		ToggleCanvasComponent& operator=(const ToggleCanvasComponent&) = delete;
		ToggleCanvasComponent(ToggleCanvasComponent&&) noexcept = default;
		ToggleCanvasComponent& operator=(ToggleCanvasComponent&&) noexcept = default;
		~ToggleCanvasComponent() override = default;

		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept override { return focusHandle_; }

		[[nodiscard]] UiToggle& Toggle() noexcept { return *toggle_; }
		[[nodiscard]] const UiToggle& Toggle() const noexcept { return *toggle_; }

		[[nodiscard]] ToggleCanvasView& View() noexcept { return *view_; }
		[[nodiscard]] const ToggleCanvasView& View() const noexcept { return *view_; }

		void SetLayoutLogicalCenterSize(float centerX, float centerY, float size) noexcept;
		void SetLayoutLogicalRect(const UiRect& r) noexcept;

		void Update(const UiInputFrame& frame, const FocusManager& focus) override;
		void SyncView() override;
		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;
		void ResetPointerInteraction() noexcept override;
		[[nodiscard]] bool IsFocusable() const noexcept override;

		void RegisterTo(UiRoot& root);

	private:
		FocusHandle focusHandle_;
		std::unique_ptr<UiToggle> toggle_;
		std::unique_ptr<ToggleCanvasView> view_;
	};
}
