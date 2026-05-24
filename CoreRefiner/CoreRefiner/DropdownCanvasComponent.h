#pragma once

#include "DropdownCanvasView.h"
#include "FocusTypes.h"
#include "IUiComponent.h"
#include "UiDropdown.h"
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
	/** @brief Dropdown Canvas 组件（Step 3：Header 可见 + 注册到 UiRoot）。 */
	class DropdownCanvasComponent final : public IUiComponent
	{
	public:
		DropdownCanvasComponent(
			Graphics& gfx,
			FocusHandle focusHandle,
			float centerX,
			float centerY,
			float width,
			float headerHeight,
			DropdownCanvasStyle style = {});

		DropdownCanvasComponent(const DropdownCanvasComponent&) = delete;
		DropdownCanvasComponent& operator=(const DropdownCanvasComponent&) = delete;
		DropdownCanvasComponent(DropdownCanvasComponent&&) noexcept = default;
		DropdownCanvasComponent& operator=(DropdownCanvasComponent&&) noexcept = default;
		~DropdownCanvasComponent() override = default;

		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept override { return focusHandle_; }

		[[nodiscard]] UiDropdown& Dropdown() noexcept { return *dropdown_; }
		[[nodiscard]] const UiDropdown& Dropdown() const noexcept { return *dropdown_; }

		[[nodiscard]] DropdownCanvasView& View() noexcept { return *view_; }
		[[nodiscard]] const DropdownCanvasView& View() const noexcept { return *view_; }

		void SetLayoutLogicalCenterSize(float centerX, float centerY, float width, float headerHeight) noexcept;

		void Update(const UiInputFrame& frame, const FocusManager& focus) override;
		void SyncView() override;
		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;
		void ResetPointerInteraction() noexcept override;
		[[nodiscard]] bool IsFocusable() const noexcept override;

		void RegisterTo(UiRoot& root);

	private:
		FocusHandle focusHandle_;
		std::unique_ptr<UiDropdown> dropdown_;
		std::unique_ptr<DropdownCanvasView> view_;
	};
}
