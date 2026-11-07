#pragma once
#include "Canvas2D.h"
#include "CanvasPixelDraw.h"
#include "IconAtlas.h"
#include "IModuleZone.h"
#include "RenderGraph.h"
#include "TextCodex.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

// 商店各块共用的纯绘制。买卖、炼成、奥义自己的排版和重画仍留在各自的块里。
namespace ModuleShopDraw
{
	// ---- 通用：钱色、按钮色、栏位方块、圆角按钮、画布、字体、贴图、矩形判断 ----

	// 货卡造价、顶栏金额、亮起的费用数字共用。
	inline constexpr Color kMoneyYellow{ 255u, 220u, 90u, 255u };

	// 炼成右侧四键和奥义区按钮共用。灭和亮成对。
	inline constexpr Color kBtnBg{ 48u, 52u, 60u, 220u };
	inline constexpr Color kBtnFrame{ 96u, 100u, 108u, 200u };
	inline constexpr Color kBtnText{ 210u, 214u, 220u, 255u };
	inline constexpr Color kBtnBgLit{ 56u, 92u, 72u, 230u };
	inline constexpr Color kBtnFrameLit{ 140u, 210u, 170u, 220u };
	inline constexpr Color kBtnTextLit{ 236u, 244u, 238u, 255u };

	// 按像素尺寸建立或撑大画布，并挂上渲染图。尺寸变了返回 true，调用方据此重画。
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

	// 点是否落在以 center 为中心、half 为半宽半高的矩形里。
	[[nodiscard]] inline bool PointInAabb_(DirectX::XMFLOAT2 center, DirectX::XMFLOAT2 half, DirectX::XMFLOAT2 pos) noexcept
	{
		return std::fabs(pos.x - center.x) <= half.x
			&& std::fabs(pos.y - center.y) <= half.y;
	}

	// 在矩形里画文字。Fixed 用 dest 做排版盒。价格和描述各自收成一条带。
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

	// 在矩形正中画一行不换行的字。
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

	// 把 16x16 图标按整数倍率贴进矩形中心，不拉长变形。
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

	// 炼成停放格和奥义栏共用的底色、边框。
	inline constexpr Color kSlotBg{ 42u, 56u, 78u, 190u };
	inline constexpr Color kSlotFrame{ 170u, 190u, 210u, 150u };

	// 画一个直角栏位方块：底色加细边。x、y 是左上角，side 是边长。side 不大于 1，或左上角在画布外时不画。
	inline void DrawSlotSquare_(Canvas2D& canvas, int x, int y, int side)
	{
		if (side <= 1 || x < 0 || y < 0)
		{
			return;
		}
		// x1、y1 是方块右下角，含端点。
		const int x1 = x + side - 1;
		const int y1 = y + side - 1;
		CanvasPixelDraw::FillRect(
			canvas,
			static_cast<unsigned>(x),
			static_cast<unsigned>(y),
			static_cast<unsigned>(x1),
			static_cast<unsigned>(y1),
			kSlotBg);
		CanvasPixelDraw::DrawRectOutline(canvas, x, y, x1, y1, kSlotFrame);
	}

	// 画一颗圆角操作按钮。x、y 是左上角，w、h 是宽高，caption 是标题。cost 小于 0 时不标价。lit 为真时按钮亮起，并且只有价格数字用 kMoneyYellow。
	inline void DrawActionButton_(
		Canvas2D& canvas,
		int x,
		int y,
		int w,
		int h,
		const std::string& caption,
		int cost,
		bool lit)
	{
		// w、h 是按钮宽高。不大于 0 时没有可画的矩形。
		if (w <= 0 || h <= 0)
		{
			return;
		}
		const int x1 = x + w - 1;
		const int y1 = y + h - 1;
		// corner 是圆角半径，随按钮高度夹在 4 到 8。
		const int corner = (std::max)(4, (std::min)(8, h / 2));
		// bg、frame、text 按是否亮起取一对颜色。
		const Color bg = lit ? kBtnBgLit : kBtnBg;
		const Color frame = lit ? kBtnFrameLit : kBtnFrame;
		const Color text = lit ? kBtnTextLit : kBtnText;
		CanvasPixelDraw::FillRoundedRect(canvas, x, y, x1, y1, corner, bg);
		CanvasPixelDraw::DrawRoundedRectOutline(canvas, x, y, x1, y1, corner, frame);

		// label 是按钮上的整行字。有费用时在标题后加空格和数字。
		std::string label = caption;
		std::vector<Text::Span> spans;
		if (cost >= 0)
		{
			const std::string digits = std::to_string(cost);
			// digitAt 是价格数字在 UTF-16 里的起点，供黄色 Span 使用。
			const UINT32 digitAt = static_cast<UINT32>(
				Utf16CodeUnitCount(label) + Utf16CodeUnitCount(" "));
			label += " " + digits;
			if (lit)
			{
				Text::Span money{};
				money.start = digitAt;
				money.length = static_cast<UINT32>(Utf16CodeUnitCount(digits));
				money.color = kMoneyYellow;
				spans.push_back(money);
			}
		}
		// font 是字号。矮按钮用 11，否则用 13。
		const float font = (h < 22) ? 11.0f : 13.0f;
		DrawShopLabelInBox_(canvas, label, font, x, y, w, h, text, spans);
	}

	// 把画布的位置和缩放对齐到这一块的世界矩形。
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

	// ---- 背景：三块底板共用的底色和边框 ----

	// 清空成商店底板色，再画一圈细边。
	inline void PaintBandChrome_(Canvas2D& canvas)
	{
		constexpr Color kBg{ 36u, 48u, 68u, 150u };
		constexpr Color kFrame{ 170u, 190u, 210u, 90u };
		canvas.Clear(kBg);
		const int w = static_cast<int>(canvas.GetCanvasWidth());
		const int h = static_cast<int>(canvas.GetCanvasHeight());
		CanvasPixelDraw::DrawRectOutline(canvas, 0, 0, w - 1, h - 1, kFrame);
	}
}
