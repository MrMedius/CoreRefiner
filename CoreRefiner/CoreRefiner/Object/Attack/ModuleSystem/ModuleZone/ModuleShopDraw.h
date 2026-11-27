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

class IModuleNode;

// 商店各块共用的颜色、笔触和排版。段落顺序是通用、背景、买卖、炼成、奥义。各块仍决定画不画、亮不亮，并读取自己的 bounds_。
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

	// 底板底色。三块共用。
	inline constexpr Color kBandBg{ 36u, 48u, 68u, 150u };
	// 底板细边。买卖顶栏的分隔线也用这个颜色。
	inline constexpr Color kBandFrame{ 170u, 190u, 210u, 90u };

	// 清空成商店底板色，再画一圈细边。
	inline void PaintBandChrome_(Canvas2D& canvas)
	{
		canvas.Clear(kBandBg);
		const int w = static_cast<int>(canvas.GetCanvasWidth());
		const int h = static_cast<int>(canvas.GetCanvasHeight());
		CanvasPixelDraw::DrawRectOutline(canvas, 0, 0, w - 1, h - 1, kBandFrame);
	}

	// ---- 买卖：顶栏、锁和货卡 ----

	// 顶栏高度。钱图标、刷新按钮和货卡的纵向位置都用它。
	inline constexpr float kTradeHudBarHeight = 32.0f;
	// 顶栏图标的世界边长。点击范围按它的一半再加 kTradeHudHitPad。
	inline constexpr float kTradeHudIconWorld = 24.0f;
	// 刷新图标和锁按钮的点击范围，在图形外再放宽这么多。
	inline constexpr float kTradeHudHitPad = 4.0f;
	// 金额、刷新价格离开图标的空隙。
	inline constexpr float kTradeHudTextGap = 4.0f;
	// 顶栏数字的字号。
	inline constexpr float kTradeHudFontSize = 14.0f;
	// 顶栏横条的填充色。底边用 kBandFrame。
	inline constexpr Color kTradeHudBar{ 28u, 38u, 56u, 200u };
	// 钱图标的颜色。和金额数字的 kMoneyYellow 不是同一个黄。
	inline constexpr Color kTradeCoin{ 255u, 210u, 80u, 255u };
	// 刷新图标：刚被拒绝、钱不够、可以刷新。
	inline constexpr Color kTradeRefreshDenied{ 230u, 80u, 80u, 255u };
	inline constexpr Color kTradeRefreshPoor{ 160u, 160u, 160u, 255u };
	inline constexpr Color kTradeRefreshOk{ 120u, 220u, 180u, 255u };

	// 锁按钮顶边离货卡底边的空隙。
	inline constexpr float kTradeLockGap = 4.0f;
	// 锁按钮画布的像素边长，以及放到世界上的边长。
	inline constexpr unsigned kTradeLockPixels = 48u;
	inline constexpr float kTradeLockWorld = 30.0f;
	// 锁图标按 16×16 放大的整数倍率。边框厚度，单位是像素。
	inline constexpr unsigned kTradeLockIconScale = 2u;
	inline constexpr unsigned kTradeLockBorder = 4u;
	// 未锁和已锁的底色、边框。图标颜色两种状态共用。
	inline constexpr Color kTradeLockBg{ 42u, 56u, 78u, 255u };
	inline constexpr Color kTradeLockBgLocked{ 22u, 30u, 42u, 255u };
	inline constexpr Color kTradeLockFrame{ 170u, 190u, 210u, 255u };
	inline constexpr Color kTradeLockFrameLocked{ 88u, 104u, 124u, 255u };
	inline constexpr Color kTradeLockIcon{ 200u, 214u, 230u, 255u };

	// 在已经铺好的底板顶上画顶栏横条。高度用 kTradeHudBarHeight。
	void PaintTradeHudBar(Canvas2D& canvas) noexcept;

	// 画一把锁。icon 是 16×16 的开锁或关锁图。locked 为真时用已锁的底色和边框。
	void PaintTradeLockButton(Canvas2D& canvas, const IconAtlas::IconBits& icon, bool locked) noexcept;

	// 把金额或刷新价格画成顶栏数字。value 是要显示的数。字号用 kTradeHudFontSize，颜色用 kMoneyYellow。
	void PaintTradeHudNumber(Canvas2D& canvas, int value) noexcept;

	// 货卡的世界宽高。纹理边长等于它们乘 kTradeCardTexelScale。摆到世界上用这两个数。
	inline constexpr float kTradeCardWidth = 160.0f;
	inline constexpr float kTradeCardHeight = 240.0f;
	// 纹理相对世界尺寸的整数倍。
	inline constexpr int kTradeCardTexelScale = 2;
	// 图标、标题和正文离开卡边的空隙。单位是世界像素，画到纹理上再乘倍数。
	inline constexpr float kTradeCardIconPad = 8.0f;
	// 货卡图标的世界半径。
	inline constexpr float kTradeCardIconRadius = 18.0f;
	// 标题和种类的字号，正文的字号，底栏价格的字号。都是世界字号，画到纹理上再乘倍数。
	inline constexpr float kTradeCardHeaderFont = 14.0f;
	inline constexpr float kTradeCardBodyFont = 13.0f;
	inline constexpr float kTradeCardPriceFont = 14.0f;
	// 标题和种类之间的行距。单位是世界像素。
	inline constexpr int kTradeCardHeaderGap = 4;
	// 已售文字的世界字号，以及它离开卡边的空隙。空隙画到纹理上再乘倍数。
	inline constexpr float kTradeCardSoldFont = 16.0f;
	inline constexpr int kTradeCardSoldPad = 6;

	// 在售底色、锁定底色、空卡底色。
	inline constexpr Color kTradeCardBg{ 42u, 56u, 78u, 190u };
	inline constexpr Color kTradeCardBgLocked{ 22u, 30u, 42u, 230u };
	inline constexpr Color kTradeCardEmptyBg{ 28u, 36u, 52u, 150u };
	// 在售边框、锁定边框。正文和价格的分隔线跟当前边框同色。
	inline constexpr Color kTradeCardFrame{ 170u, 190u, 210u, 150u };
	inline constexpr Color kTradeCardFrameLocked{ 88u, 104u, 124u, 210u };
	// 已售文字的颜色。
	inline constexpr Color kTradeCardSold{ 160u, 160u, 160u, 220u };

	// 铺货卡底色并画一圈边。empty 为真时用空卡底色。locked 为真且不是空卡时用锁定底色和锁定边。
	void PaintTradeCardChrome(Canvas2D& canvas, bool empty, bool locked) noexcept;

	// 画已售卡。先铺空卡底，再把 SOLD OUT 放在正中。
	void PaintTradeCardSold(Canvas2D& canvas);

	// 画在售货卡。node 提供标题、种类和正文。price 是底栏金额，用 kMoneyYellow。locked 为真时用锁定底色和边。
	void PaintTradeCardStock(Canvas2D& canvas, const IModuleNode& node, int price, bool locked);

	// ---- 炼成：三格和右侧四键的像素排版 ----

	// 停放格个数。与 ModuleShopRefine::kParkCount 相同。
	inline constexpr int kRefineSlotCount = 3;
	// 右侧按钮个数。与 ModuleShopRefine::kButtonCount 相同。
	inline constexpr int kRefineButtonCount = 4;

	// 内边距的下限。画布更高时改用高度除以 kRefinePadDiv。
	inline constexpr int kRefinePadMin = 8;
	// 用画布高度算内边距时的除数。
	inline constexpr int kRefinePadDiv = 14;
	// 格与格、格与按钮之间希望留下的空隙。
	inline constexpr int kRefineRegionGapWanted = 64;
	// 空隙再窄也不低于这个值。
	inline constexpr int kRefineRegionGapMin = 32;
	// 四键之间的竖直间距。
	inline constexpr int kRefineButtonGap = 5;
	// 四键希望的宽度。放不下时再缩小，但不低于 kRefineButtonWMin。
	inline constexpr int kRefineButtonW = 120;
	// 四键再矮也不低于这个高度。
	inline constexpr int kRefineButtonHMin = 14;
	// 四键再窄也不低于这个宽度。
	inline constexpr int kRefineButtonWMin = 72;
	// 格子边长再缩也不低于这个值。
	inline constexpr int kRefineSideMin = 24;
	// 图标半径相对格边的比例。和仓库格距 56、半径 15 相同。货卡不用这个数。
	inline constexpr float kRefineIconToSlot = 15.0f / 56.0f;
	// 图标世界半径的下限。
	inline constexpr float kRefineIconRadiusMin = 8.0f;

	// 三格和四键在画布上的像素位置。不含世界矩形。
	struct RefineLayout
	{
		// 内容区离开画布边缘的像素。
		int pad = 0;
		// 每一格的边长。
		int side = 0;
		// 三格共同的顶边。
		int slotTop = 0;
		// 三格的左边。0 素材，1 主体，2 结果。
		int slotXs[kRefineSlotCount]{};
		// 右侧按钮的宽、高、左边、顶边和间距。
		int btnW = kRefineButtonW;
		int btnH = kRefineButtonHMin;
		int btnX = 0;
		int btnTop = 0;
		int btnGap = kRefineButtonGap;
	};

	// 按画布像素宽高排三格和四键。cw、ch 是宽和高。不读世界矩形。
	void FillRefineLayout(int cw, int ch, RefineLayout& layout) noexcept;

	// worldSide 是格子边长换到世界上的长度。半径用 kRefineIconToSlot，至少 kRefineIconRadiusMin。
	[[nodiscard]] inline float RefineSlotIconRadius(float worldSide) noexcept
	{
		return (std::max)(kRefineIconRadiusMin, worldSide * kRefineIconToSlot);
	}

	// ---- 奥义：栏位、右侧按钮、锻造键和翻页条 ----

	// 有奥义时右侧按钮的个数。锻造单独画在正中，不占这三颗。
	inline constexpr int kUltraButtonCount = 3;
	// 内部栏边长 = 奥义栏边长 * kUltraInnerNumer / kUltraInnerDenom。顶边与奥义栏齐平。
	inline constexpr int kUltraInnerNumer = 3;
	inline constexpr int kUltraInnerDenom = 4;

	// 内边距的下限。画布更高时改用高度除以 kUltraPadDiv。
	inline constexpr int kUltraPadMin = 6;
	inline constexpr int kUltraPadDiv = 16;
	// 右侧三键的竖直间距。
	inline constexpr int kUltraButtonGap = 4;
	// 右侧按钮宽度夹在这两个值之间，再按内容宽度除以 kUltraButtonWDiv 取一档。
	inline constexpr int kUltraButtonWMin = 72;
	inline constexpr int kUltraButtonWMax = 120;
	inline constexpr int kUltraButtonWDiv = 5;
	// 右侧按钮再矮也不低于这个高度。
	inline constexpr int kUltraButtonHMin = 14;
	// 奥义栏和内部栏之间的空隙下限。更宽时取内边距的一半。
	inline constexpr int kUltraInnerGapMin = 4;
	// 翻页条至少要这么高，放得下可点的三角。不够时两栏一起缩小。
	inline constexpr int kUltraPagerMinH = 16;
	// 为翻页条缩小栏位时，奥义栏边长不低于这个值。
	inline constexpr int kUltraSideFloor = 8;
	// 右侧按钮左边的剩余宽度高于这个值，才按比例缩小两栏。
	inline constexpr int kUltraRoomMin = 8;
	// 窄宽度下奥义栏边长的下限。
	inline constexpr int kUltraFittedMin = 24;

	// 没有奥义时，正中锻造键的宽高范围。
	inline constexpr int kUltraForgeWMin = 72;
	inline constexpr int kUltraForgeWMax = 168;
	inline constexpr int kUltraForgeWDiv = 4;
	inline constexpr int kUltraForgeHMin = 18;
	inline constexpr int kUltraForgeHMax = 40;
	inline constexpr int kUltraForgeHDiv = 3;
	// 锻造键离开画布边缘至少这么多像素。
	inline constexpr int kUltraForgeMargin = 8;

	// 翻页三角的最大高度。
	inline constexpr int kUltraPagerTriMax = 16;
	// 条带宽或高低于这两个值时不画三角和圆点。
	inline constexpr int kUltraPagerMinW = 8;
	inline constexpr int kUltraPagerMinDrawH = 6;
	// 三角高度用条高减去这个边，宽度至少这么多。
	inline constexpr int kUltraPagerTriInset = 4;
	inline constexpr int kUltraPagerTriWMin = 8;
	// 圆点半径夹在这两个值之间，再按三角高度除以 kUltraPagerDotDiv 缩小。
	inline constexpr int kUltraPagerDotRadiusMin = 2;
	inline constexpr int kUltraPagerDotRadiusMax = 4;
	inline constexpr int kUltraPagerDotDiv = 5;
	// 圆点间距至少这么多，也不小于半径。
	inline constexpr int kUltraPagerDotGapMin = 3;

	// 图标半径相对格边的比例。和炼成、仓库相同。货卡不用这个数。
	inline constexpr float kUltraIconToSlot = 15.0f / 56.0f;
	// 图标世界半径的下限。
	inline constexpr float kUltraIconRadiusMin = 8.0f;

	// 奥义栏、内部栏、右侧按钮、锻造键和翻页条的像素位置。不含世界矩形，也不含当前页。
	struct UltraLayout
	{
		// 内容区离开画布边缘的像素。
		int pad = 0;
		// 奥义栏的左边、顶边和边长。
		int ultraX = 0;
		int ultraY = 0;
		int ultraSide = 0;
		// 内部栏的边长、间距、第一格左边和顶边。
		int innerSide = 0;
		int innerGap = 0;
		int innerX0 = 0;
		int innerY = 0;
		// 内部栏容量。没有奥义时是 0。由奥义区传入。
		int capacity = 0;
		// 右侧按钮左边这一屏能放下几格，不按容量截断。
		int windowCount = 0;
		// 右侧三键的左边、顶边、宽、高和间距。
		int btnX = 0;
		int btnY = 0;
		int btnW = 0;
		int btnH = 0;
		int btnGap = 0;
		// 没有奥义时，正中锻造键的位置和大小。
		int forgeX = 0;
		int forgeY = 0;
		int forgeW = 0;
		int forgeH = 0;
		// 内部栏下面、奥义栏和右侧按钮之间的翻页条。
		int pagerX = 0;
		int pagerY = 0;
		int pagerW = 0;
		int pagerH = 0;
	};

	// 翻页条上左三角、圆点和右三角的像素位置。圆点不能点。
	struct UltraPagerGlyphs
	{
		// 条带太窄或容量为 0 时为 false，调用方不画也不响应。
		bool valid = false;
		// 左三角、右三角的左边，以及三角的顶边、宽、高。
		int leftX = 0;
		int rightX = 0;
		int triY = 0;
		int triW = 0;
		int triH = 0;
		// 圆点个数，等于容量。
		int count = 0;
		// 圆点半径、间距、第一个圆心和竖直中心。stride 是相邻圆心的水平距离。
		int radius = 0;
		int gap = 0;
		int firstCx = 0;
		int cy = 0;
		int stride = 0;
	};

	// 按画布宽高和容量排栏位、按钮和翻页条。cw、ch 是宽和高。capacity 是内部栏个数。不读世界矩形，也不决定当前页。
	void FillUltraLayout(int cw, int ch, int capacity, UltraLayout& layout) noexcept;

	// 按翻页条矩形和容量排出三角和圆点。capacity 是圆点个数。
	[[nodiscard]] UltraPagerGlyphs BuildUltraPagerGlyphs(int pagerX, int pagerY, int pagerW, int pagerH, int capacity) noexcept;

	// 画翻页三角和圆点。visibleBegin 是当前页第一格，visibleCount 是这一页画出来的格数。到头的三角不亮。
	void DrawUltraPager(Canvas2D& canvas, const UltraLayout& layout, int visibleBegin, int visibleCount) noexcept;

	// worldSide 是栏位边长换到世界上的长度。半径用 kUltraIconToSlot，至少 kUltraIconRadiusMin。
	[[nodiscard]] inline float UltraSlotIconRadius(float worldSide) noexcept
	{
		return (std::max)(kUltraIconRadiusMin, worldSide * kUltraIconToSlot);
	}
}
