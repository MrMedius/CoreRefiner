#pragma once

#include "FocusTypes.h"
#include "IUiComponent.h"
#include "ProgressBarCanvasView.h"
#include "UiProgressBar.h"
#include "UiRoot.h"
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
	class ProgressBarCanvasComponent final : public IUiComponent
	{
	public:
		ProgressBarCanvasComponent(
			Graphics& gfx,
			unsigned canvasPixelWidth,
			unsigned canvasPixelHeight,
			ProgressBarCanvasStyle style = {});

		ProgressBarCanvasComponent(
			Graphics& gfx,
			float centerX,
			float centerY,
			float width,
			float height,
			ProgressBarCanvasStyle style = {});

		ProgressBarCanvasComponent(const ProgressBarCanvasComponent&) = delete;
		ProgressBarCanvasComponent& operator=(const ProgressBarCanvasComponent&) = delete;
		ProgressBarCanvasComponent(ProgressBarCanvasComponent&&) noexcept = default;
		ProgressBarCanvasComponent& operator=(ProgressBarCanvasComponent&&) noexcept = default;
		~ProgressBarCanvasComponent() override = default;

		[[nodiscard]] UiProgressBar& ProgressBar() noexcept { return *progressBar_; }
		[[nodiscard]] const UiProgressBar& ProgressBar() const noexcept { return *progressBar_; }

		[[nodiscard]] ProgressBarCanvasView& View() noexcept { return *view_; }
		[[nodiscard]] const ProgressBarCanvasView& View() const noexcept { return *view_; }

		void SetLayoutLogicalCenterSize(float centerX, float centerY, float width, float height) noexcept;

		void Update(const UiInputFrame& frame, const FocusManager& focus) override;
		void SyncView() override;
		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;
		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept override { return kInvalidFocusHandle; }
		void ResetPointerInteraction() noexcept override {}
		[[nodiscard]] bool IsFocusable() const noexcept override { return false; }

		void RegisterTo(UiRoot& root);

	private:
		std::unique_ptr<UiProgressBar> progressBar_;
		std::unique_ptr<ProgressBarCanvasView> view_;
	};
}
