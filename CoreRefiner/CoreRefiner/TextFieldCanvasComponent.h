#pragma once

#include "TextFieldCanvasView.h"
#include "FocusTypes.h"
#include "IUiComponent.h"
#include "UiTextField.h"
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
	class TextFieldCanvasComponent final : public IUiComponent
	{
	public:
		TextFieldCanvasComponent(
			Graphics& gfx,
			FocusHandle focusHandle,
			unsigned canvasPixelWidth,
			unsigned canvasPixelHeight,
			TextFieldCanvasStyle style = {});

		TextFieldCanvasComponent(
			Graphics& gfx,
			FocusHandle focusHandle,
			float centerX,
			float centerY,
			float width,
			float height,
			TextFieldCanvasStyle style = {});

		TextFieldCanvasComponent(const TextFieldCanvasComponent&) = delete;
		TextFieldCanvasComponent& operator=(const TextFieldCanvasComponent&) = delete;
		TextFieldCanvasComponent(TextFieldCanvasComponent&&) noexcept = default;
		TextFieldCanvasComponent& operator=(TextFieldCanvasComponent&&) noexcept = default;
		~TextFieldCanvasComponent() override = default;

		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept override { return focusHandle_; }

		[[nodiscard]] UiTextField& Field() noexcept { return *field_; }
		[[nodiscard]] const UiTextField& Field() const noexcept { return *field_; }

		[[nodiscard]] TextFieldCanvasView& View() noexcept { return *view_; }
		[[nodiscard]] const TextFieldCanvasView& View() const noexcept { return *view_; }

		void SetLayoutLogicalCenterSize(float centerX, float centerY, float width, float height) noexcept;
		void SetLayoutLogicalRect(const UiRect& r) noexcept;

		void Update(const UiInputFrame& frame, FocusManager& focus) override;
		void SyncView() override;
		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;
		void ResetPointerInteraction() noexcept override;
		[[nodiscard]] bool IsFocusable() const noexcept override;
		[[nodiscard]] bool ConsumesDirectionalNavigation() const noexcept override;
		[[nodiscard]] bool ConsumesTextInput() const noexcept override;

		void RegisterTo(UiRoot& root);

	private:
		FocusHandle focusHandle_;
		std::unique_ptr<UiTextField> field_;
		std::unique_ptr<TextFieldCanvasView> view_;
	};
}
