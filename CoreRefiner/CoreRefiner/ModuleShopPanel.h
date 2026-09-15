#pragma once

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

	static constexpr float kWidth = 160.0f;
	static constexpr float kHeight = 240.0f;
	// 纹理相对世界尺寸的整数倍；SetScale 仍用 kWidth×kHeight。
	static constexpr int kTexelScale = 2;
	static constexpr float kIconPad = 8.0f;
	// 货卡 Icon 独一层居中，不再跟标题+Kind 字高绑定。
	static constexpr float kIconWorldRadius = 18.0f;
	static constexpr float kHeaderFontSize = 14.0f;
	static constexpr float kBodyFontSize = 13.0f;
	static constexpr float kPriceFontSize = 14.0f;
	static constexpr int kHeaderLineGap = 4;

	[[nodiscard]] static int TexelScale() noexcept;
	[[nodiscard]] static int IconSidePx() noexcept;
	[[nodiscard]] static float IconWorldRadius() noexcept { return kIconWorldRadius; }

	void Ensure(Graphics& gfx, Rgph::RenderGraph& rg);
	void Rebuild(const IModuleNode* node, int price, bool sold, bool locked);
	void SetWorldCenter(DirectX::XMFLOAT3 center) noexcept;
	void Submit() const;

private:
	void PaintChrome_(bool empty, bool locked);
	void PaintSold_();
	void PaintStock_(const IModuleNode& node, int price, bool locked);

	Graphics* gfx_{ nullptr };
	Rgph::RenderGraph* rg_{ nullptr };
	std::unique_ptr<Canvas2D> canvas_;
};
