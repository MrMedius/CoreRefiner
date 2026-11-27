#pragma once

#include "DropdownCanvasView.h"
#include "FocusTypes.h"
#include "IUiComponent.h"
#include "IUiPopupConsumer.h"
#include "UiDropdown.h"
#include "UiRoot.h"
#include "UiTypes.h"

#include <DirectXMath.h>
#include <functional>
#include <memory>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

namespace Ui
{
	// Dropdown Canvas 组件（Header + 展开列表 + Modal Popup）。
	class DropdownCanvasComponent final : public IUiComponent, public IUiPopupConsumer
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

		void SetOptions(std::vector<UiOption> options);
		void AddOptions(std::vector<UiOption> options);
		void AddOption(UiOption option);
		void EraseOptions(std::vector<int> indices);
		void EraseOption(int index);
		void ClearOptions() noexcept;

		void BindOptionList(UiOptionList& list) noexcept;
		void UnbindOptionList() noexcept;
		[[nodiscard]] UiOptionList& OptionList() noexcept { return dropdown_->OptionList(); }
		[[nodiscard]] const UiOptionList& OptionList() const noexcept { return dropdown_->OptionList(); }

		void SetOnValueChanged(std::function<void(int index, const std::string& label)> cb);

		void Update(const UiInputFrame& frame, FocusManager& focus) override;
		void SyncView() override;
		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;
		void ResetPointerInteraction() noexcept override;
		[[nodiscard]] bool IsFocusable() const noexcept override;
		[[nodiscard]] bool ConsumesDirectionalNavigation() const noexcept override;

		[[nodiscard]] bool IsPopupOpen() const noexcept override;

		[[nodiscard]] bool BlocksUnderlyingPointerAt(float x, float y) const noexcept override;

		void OnPopupInput(UiInputFrame& frame, FocusManager& focus) override;

		void RegisterTo(UiRoot& root);

	private:
		FocusHandle focusHandle_;
		std::unique_ptr<UiDropdown> dropdown_;
		std::unique_ptr<DropdownCanvasView> view_;
	};
}
