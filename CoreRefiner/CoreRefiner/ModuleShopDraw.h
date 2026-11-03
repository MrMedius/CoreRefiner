#pragma once
#include "Canvas2D.h"
#include "CanvasPixelDraw.h"
#include "IconAtlas.h"
#include "IModuleZone.h"
#include "RenderGraph.h"
#include "TextCodex.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

// 商店各区块共用的绘制。炼成按钮的亮灭色也放在这里，奥义区按钮沿用同一套。
namespace ModuleShopDraw
{
	// 货卡造价、顶栏金额、亮起的费用数字共用。
	inline constexpr Color kMoneyYellow{ 255u, 220u, 90u, 255u };

	// 炼成右侧四键。灭和亮成对。
	inline constexpr Color kBtnBg{ 48u, 52u, 60u, 220u };
	inline constexpr Color kBtnFrame{ 96u, 100u, 108u, 200u };
	inline constexpr Color kBtnText{ 210u, 214u, 220u, 255u };
	inline constexpr Color kBtnBgLit{ 56u, 92u, 72u, 230u };
	inline constexpr Color kBtnFrameLit{ 140u, 210u, 170u, 220u };
	inline constexpr Color kBtnTextLit{ 236u, 244u, 238u, 255u };

	inline bool EnsureCanvasSize_(std::unique_ptr<Canvas2D>& canvas, Graphics& gfx, Rgph::RenderGraph& rg, unsigned w, unsigned h)
	{
		if (canvas == nullptr)
		{
			canvas = std::make_unique<Canvas2D>(gfx, w, h);
			canvas->LinkTechniques(rg);
			return true;
		}
		if (canvas->GetCanvasWidth() != w || canvas->GetCanvasHeight() != h)
		{
			canvas->Resize(w, h);
			return true;
		}
		return false;
	}

	inline void PaintBandChrome_(Canvas2D& canvas)
	{
		constexpr Color kBg{ 36u, 48u, 68u, 150u };
		constexpr Color kFrame{ 170u, 190u, 210u, 90u };
		canvas.Clear(kBg);
		const int w = static_cast<int>(canvas.GetCanvasWidth());
		const int h = static_cast<int>(canvas.GetCanvasHeight());
		CanvasPixelDraw::DrawRectOutline(canvas, 0, 0, w - 1, h - 1, kFrame);
	}

	// 商店 UI 共用字体栈：YaHei，回退 Yu Gothic / Segoe。
	inline void FillUiLabelRequest_(Text::RenderRequest& rq, const std::string& text, float fontSize, Color color)
	{
		rq.text = text;
		rq.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
		rq.fallbackFonts.clear();
		rq.fallbackFonts.push_back(Text::FontSource::System(L"Yu Gothic UI"));
		rq.fallbackFonts.push_back(Text::FontSource::System(L"Segoe UI"));
		rq.style.fontSize = fontSize;
		rq.defaultColor = color;
	}

	[[nodiscard]] inline bool PointInAabb_(DirectX::XMFLOAT2 center, DirectX::XMFLOAT2 half, DirectX::XMFLOAT2 pos) noexcept
	{
		return std::fabs(pos.x - center.x) <= half.x
			&& std::fabs(pos.y - center.y) <= half.y;
	}

	// Fixed 用 dest 做排版盒（不再认 maxWidthPx）。价格/描述各自收成一条带。
	inline void DrawShopText_(
		Canvas2D& canvas,
		const std::string& text,
		float fontSize,
		int boxX,
		int boxY,
		int boxW,
		int boxH,
		int paddingPx,
		DWRITE_TEXT_ALIGNMENT align,
		DWRITE_PARAGRAPH_ALIGNMENT para,
		Color color,
		const std::vector<Text::Span>& spans = {},
		bool wrap = true)
	{
		if (text.empty() || boxW <= 0 || boxH <= 0)
		{
			return;
		}

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		FillUiLabelRequest_(rq, text, fontSize, color);
		rq.canvasMode = Text::CanvasMode::Fixed;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.style.wordWrapEnabled = wrap;
		rq.style.textAlign = align;
		rq.style.paragraphAlign = para;
		rq.paddingPx = paddingPx;
		rq.SetDestRect(
			static_cast<float>(boxX),
			static_cast<float>(boxY),
			static_cast<float>(boxW),
			static_cast<float>(boxH));
		rq.backgroundColor = Colors::None;
		rq.spans = spans;
		ctx.Render(canvas);
	}

	inline void DrawShopLabelInBox_(
		Canvas2D& canvas,
		const std::string& text,
		float fontSize,
		int boxX,
		int boxY,
		int boxW,
		int boxH,
		Color color,
		const std::vector<Text::Span>& spans = {})
	{
		if (text.empty() || boxW <= 0 || boxH <= 0)
		{
			return;
		}

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		FillUiLabelRequest_(rq, text, fontSize, color);
		rq.canvasMode = Text::CanvasMode::Fixed;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.style.wordWrapEnabled = false;
		rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_CENTER;
		rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		rq.paddingPx = 1;
		rq.SetDestRect(
			static_cast<float>(boxX),
			static_cast<float>(boxY),
			static_cast<float>(boxW),
			static_cast<float>(boxH));
		rq.backgroundColor = Colors::None;
		rq.spans = spans;
		ctx.Render(canvas);
	}

	// 整数倍率贴进矩形中心，不拉长变形。
	inline void BlitIconInBox_(
		Canvas& dest,
		const IconAtlas::IconBits& bits,
		Color color,
		int boxX,
		int boxY,
		int boxW,
		int boxH)
	{
		if (boxW <= 0 || boxH <= 0)
		{
			return;
		}

		const int scale = (std::max)(1, (std::min)(boxW, boxH) / 16);
		const int glyph = 16 * scale;
		const int ox = boxX + (boxW - glyph) / 2;
		const int oy = boxY + (boxH - glyph) / 2;
		for (unsigned y = 0u; y < 16u; ++y)
		{
			const std::uint16_t row = bits[y];
			for (unsigned x = 0u; x < 16u; ++x)
			{
				if ((row & static_cast<std::uint16_t>(1u << (15u - x))) == 0)
				{
					continue;
				}
				for (int sy = 0; sy < scale; ++sy)
				{
					for (int sx = 0; sx < scale; ++sx)
					{
						CanvasPixelDraw::PutPixelClamped(
							dest,
							ox + static_cast<int>(x) * scale + sx,
							oy + static_cast<int>(y) * scale + sy,
							color);
					}
				}
			}
		}
	}

	inline void SyncBandTransform_(Canvas2D* canvas, const IModuleZone::BoundsWorld& b)
	{
		if (canvas == nullptr)
		{
			return;
		}
		canvas->SetPosition(DirectX::XMFLOAT3{ b.center.x, b.center.y, 0.0f });
		canvas->SetScale(DirectX::XMFLOAT3{
			b.half.x * 2.0f,
			b.half.y * 2.0f,
			1.0f
		});
	}
}
