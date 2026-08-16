#include "NodeInfoPanel.h"

#include "Canvas2D.h"
#include "Channels.h"
#include "Colors.h"
#include "Graphics.h"
#include "RenderGraph.h"
#include "TextCodex.h"

#include <algorithm>

namespace
{
	[[nodiscard]] DirectX::XMFLOAT2 ClampPanelCenter_(
		float cx,
		float cy,
		float halfW,
		float halfH,
		float pad) noexcept
	{
		const float minX = pad + halfW;
		const float maxX = static_cast<float>(SCREEN_WIDTH) - pad - halfW;
		const float minY = pad + halfH;
		const float maxY = static_cast<float>(SCREEN_HEIGHT) - pad - halfH;

		if (minX <= maxX)
		{
			cx = std::clamp(cx, minX, maxX);
		}
		else
		{
			cx = static_cast<float>(SCREEN_WIDTH) * 0.5f;
		}

		if (minY <= maxY)
		{
			cy = std::clamp(cy, minY, maxY);
		}
		else
		{
			cy = static_cast<float>(SCREEN_HEIGHT) * 0.5f;
		}

		return DirectX::XMFLOAT2{ cx, cy };
	}
}

void NodeInfoPanel::Ensure(Graphics& gfx, Rgph::RenderGraph& rg)
{
	gfx_ = &gfx;
	rg_ = &rg;
	if (canvas_ != nullptr)
	{
		return;
	}

	constexpr unsigned kBoot = 64u;
	canvas_ = std::make_unique<Canvas2D>(gfx, kBoot, kBoot);
	canvas_->Clear(Colors::None);
	canvas_->LinkTechniques(rg);
}

void NodeInfoPanel::ShowFor(ModuleNodeLabel label, DirectX::XMFLOAT2 anchorGameXY, Anchor anchor, float maxWidthPx, bool titleOnly)
{
	if (canvas_ == nullptr)
	{
		return;
	}

	if (maxWidthPx <= 0.0f)
	{
		maxWidthPx = kMaxWidthPx_;
	}

	const ModuleNodeInfoLanguage lang = GetModuleNodeInfoLanguage();
	const bool contentDirty =
		!cachedLabel_.has_value()
		|| !cachedLanguage_.has_value()
		|| *cachedLabel_ != label
		|| *cachedLanguage_ != lang
		|| maxWidthPx_ != maxWidthPx
		|| titleOnly_ != titleOnly;

	anchor_ = anchor;
	maxWidthPx_ = maxWidthPx;
	titleOnly_ = titleOnly;

	if (contentDirty)
	{
		RebuildContent_(label);
		cachedLabel_ = label;
		cachedLanguage_ = lang;
	}

	visible_ = true;
	SyncPosition_(anchorGameXY);
}

void NodeInfoPanel::Hide() noexcept
{
	visible_ = false;
}

void NodeInfoPanel::Submit() const
{
	if (!visible_ || canvas_ == nullptr)
	{
		return;
	}
	canvas_->Submit(Chan::ui);
}

void NodeInfoPanel::RebuildContent_(ModuleNodeLabel label)
{
	const ModuleNodeInfoEntry& entry = GetModuleNodeInfoCopy(label);
	std::string text = titleOnly_ ? entry.title : entry.ComposedText();
	if (text.empty())
	{
		text = entry.body;
	}

	auto ctx = TextCodex::Get().BeginDraw();
	Text::RenderRequest& rq = ctx.Request();
	rq.text = text;
	rq.canvasMode = Text::CanvasMode::Auto;
	rq.clearMode = Text::ClearMode::Clear;
	rq.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
	rq.fallbackFonts.clear();
	rq.fallbackFonts.push_back(Text::FontSource::System(L"Yu Gothic UI"));
	rq.fallbackFonts.push_back(Text::FontSource::System(L"Segoe UI"));
	rq.style.fontSize = kFontSize_;
	rq.style.wordWrapEnabled = true;
	rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
	rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
	rq.maxWidthPx = maxWidthPx_;
	rq.paddingPx = kPaddingPx_;
	rq.defaultColor = Colors::White;
	rq.backgroundColor = Color{ 24u, 26u, 32u, 220u };
	rq.spans = entry.spans;

	const Text::MeasureResult measure = ctx.Measure();
	contentW_ = (std::max)(1u, measure.widthPx);
	contentH_ = (std::max)(1u, measure.heightPx);

	ctx.Render(*canvas_);

	contentW_ = (std::max)(1u, canvas_->GetCanvasWidth());
	contentH_ = (std::max)(1u, canvas_->GetCanvasHeight());

	canvas_->NotifyPixelsChanged();
	canvas_->SetScale(DirectX::XMFLOAT3{
		static_cast<float>(contentW_),
		static_cast<float>(contentH_),
		1.0f
	});
}

void NodeInfoPanel::SyncPosition_(DirectX::XMFLOAT2 anchorGameXY)
{
	if (canvas_ == nullptr)
	{
		return;
	}

	const float halfW = static_cast<float>(contentW_) * 0.5f;
	const float halfH = static_cast<float>(contentH_) * 0.5f;

	const float preferCx = anchorGameXY.x;
	const float preferCy = (anchor_ == Anchor::Below)
		? (anchorGameXY.y + halfH + kAnchorGap_)
		: (anchorGameXY.y - halfH - kAnchorGap_);
	const DirectX::XMFLOAT2 center = ClampPanelCenter_(
		preferCx,
		preferCy,
		halfW,
		halfH,
		kScreenPad_);

	canvas_->SetPosition(DirectX::XMFLOAT3{ center.x, center.y, 0.0f });
}
