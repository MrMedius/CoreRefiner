#pragma once

#include "DropdownCanvasTypes.h"
#include "DropdownListItemViewModel.h"

#include "Canvas2D.h"

#include <memory>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

namespace Ui
{
	/** @brief Dropdown 列表单行 Canvas 视图（bg + focus ring + text）。 */
	class DropdownListItemCanvasView
	{
	public:
		DropdownListItemCanvasView(
			Graphics& gfx,
			unsigned pixelWidth,
			unsigned pixelHeight,
			const DropdownCanvasStyle& style);

		void SyncFrom(const DropdownListItemViewModel& vm);
		void ApplyLayout(float centerX, float centerY, float width, float height) noexcept;
		void LinkTechniques(Rgph::RenderGraph& rg);
		void Submit(std::size_t channelMask) const;

	private:
		[[nodiscard]] Color BackgroundForRow_(const DropdownListItemViewModel& vm) const noexcept;
		void RepaintBackground_(const DropdownListItemViewModel& vm);
		void RepaintFocusRing_(const DropdownListItemViewModel& vm);
		void RepaintText_(const DropdownListItemViewModel& vm);

		DropdownCanvasStyle style_;
		unsigned pixelWidth_ = 1u;
		unsigned pixelHeight_ = 1u;
		std::unique_ptr<Canvas2D> bgCanvas_;
		std::unique_ptr<Canvas2D> ringCanvas_;
		std::unique_ptr<Canvas2D> textCanvas_;

		DropdownListItemViewModel lastPainted_{};
		bool hasPainted_ = false;
	};
}
