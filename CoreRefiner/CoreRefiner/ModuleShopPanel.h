#pragma once

#include "ModuleShopDraw.h"

#include <DirectXMath.h>
#include <memory>

class Canvas2D;
class Graphics;
class IModuleNode;

namespace Rgph
{
	class RenderGraph;
}

// 买卖货卡面：标题 / Kind / 描述 / 标价。不负责进货、锁、拖放。
class ModuleShopPanel
{	
public:
	ModuleShopPanel() = default;
	~ModuleShopPanel();

	ModuleShopPanel(const ModuleShopPanel&) = delete;
	ModuleShopPanel& operator=(const ModuleShopPanel&) = delete;

	// 货卡世界宽度。数值在 ModuleShopDraw::kTradeCardWidth。
	static constexpr float kWidth = ModuleShopDraw::kTradeCardWidth;
	// 货卡世界高度。数值在 ModuleShopDraw::kTradeCardHeight。
	static constexpr float kHeight = ModuleShopDraw::kTradeCardHeight;
	// 纹理相对世界尺寸的整数倍。摆到世界上仍用 kWidth 和 kHeight。数值在 kTradeCardTexelScale。
	static constexpr int kTexelScale = ModuleShopDraw::kTradeCardTexelScale;
	// 图标和文字离开卡边的空隙。数值在 kTradeCardIconPad。
	static constexpr float kIconPad = ModuleShopDraw::kTradeCardIconPad;
	// 货卡图标的世界半径。数值在 kTradeCardIconRadius。
	static constexpr float kIconWorldRadius = ModuleShopDraw::kTradeCardIconRadius;
	// 标题和种类的字号。数值在 kTradeCardHeaderFont。
	static constexpr float kHeaderFontSize = ModuleShopDraw::kTradeCardHeaderFont;
	// 正文的字号。数值在 kTradeCardBodyFont。
	static constexpr float kBodyFontSize = ModuleShopDraw::kTradeCardBodyFont;
	// 底栏价格的字号。数值在 kTradeCardPriceFont。
	static constexpr float kPriceFontSize = ModuleShopDraw::kTradeCardPriceFont;
	// 标题和种类之间的行距。数值在 kTradeCardHeaderGap。
	static constexpr int kHeaderLineGap = ModuleShopDraw::kTradeCardHeaderGap;

	// 纹理倍数，至少为 1。
	[[nodiscard]] static int TexelScale() noexcept;
	// 图标在纹理上的边长。直径是世界半径乘纹理倍数。
	[[nodiscard]] static int IconSidePx() noexcept;
	// 货卡图标的世界半径。
	[[nodiscard]] static float IconWorldRadius() noexcept { return kIconWorldRadius; }

	// 建画布并接到渲染图。宽高变化时重开纹理。
	void Ensure(Graphics& gfx, Rgph::RenderGraph& rg);
	// 按货的状态重画。sold 画已售，node 为空画空卡，否则画在售。price 是标价，locked 决定是否用锁定配色。
	void Rebuild(const IModuleNode* node, int price, bool sold, bool locked);
	// 把画布中心放到世界坐标。缩放用货卡世界宽高。
	void SetWorldCenter(DirectX::XMFLOAT3 center) noexcept;
	// 交到界面通道。
	void Submit() const;

private:
	// 画面设备和渲染图。Ensure 时记下，用来建画布。
	Graphics* gfx_{ nullptr };
	Rgph::RenderGraph* rg_{ nullptr };
	// 这一张货卡的画布。
	std::unique_ptr<Canvas2D> canvas_;
};
