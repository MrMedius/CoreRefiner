#pragma once

#include "FocusManager.h"
#include "MouseUiInputAdapter.h"

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

	/**
	 * @struct UiButtonSlot
	 * @brief 非拥有：由外层持有 `UiButton` 与 `IButtonView` 生命周期。
	 */
	struct UiButtonSlot
	{
		UiButton* button = nullptr;
		IButtonView* view = nullptr;
	};

	/**
	 * @class UiRoot
	 * @brief 单帧 UI 流水线：合并语义输入、焦点、Update、Sync、Submit。
	 *
	 * @par 调用约定
	 * - `TickAfterInput()`：同帧须在 `InputCodex::Update()` **之后**调用。
	 * - `Submit()`：在 UI `RenderGraph::Execute` **之前**调用（例如 `Chan::ui`）。
	 */
	class UiRoot
	{
	public:
		UiRoot() = default;

		void Clear() noexcept;

		void AddButtonSlot(UiButton* btn, IButtonView* view);
		void RebuildTabOrderFromSlots();

		void InitLinkTechniques(Rgph::RenderGraph& rg);

		void TickAfterInput();
		void Submit(std::size_t channelMask) const;

		[[nodiscard]] FocusManager& Focus() noexcept { return focus_; }
		[[nodiscard]] const FocusManager& Focus() const noexcept { return focus_; }

	private:
		[[nodiscard]] UiInputFrame BuildMergedInputFrame_() const;

		FocusManager focus_{};
		MouseUiInputAdapter mouse_{};
		std::vector<UiButtonSlot> slots_{};
	};
}