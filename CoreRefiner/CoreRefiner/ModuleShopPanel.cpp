#include "ModuleShopPanel.h"

#include "Canvas2D.h"
#include "Channels.h"
#include "Graphics.h"
#include "IModuleNode.h"
#include "RenderGraph.h"

#include <algorithm>
#include <cmath>

ModuleShopPanel::~ModuleShopPanel() = default;

int ModuleShopPanel::TexelScale() noexcept
{
	return (std::max)(1, kTexelScale);
}

int ModuleShopPanel::IconSidePx() noexcept
{
	// n 是纹理倍数。边长是直径，所以半径乘 2。
	const float n = static_cast<float>(TexelScale());
	return (std::max)(1, static_cast<int>(std::lround(2.0f * kIconWorldRadius * n)));
}

void ModuleShopPanel::Ensure(Graphics& gfx, Rgph::RenderGraph& rg)
{
	gfx_ = &gfx;
	rg_ = &rg;
	// n 是纹理倍数。w、h 是画布像素，等于世界宽高乘倍数。
	const unsigned n = static_cast<unsigned>(TexelScale());
	const unsigned w = (std::max)(1u, static_cast<unsigned>(std::lround(kWidth)) * n);
	const unsigned h = (std::max)(1u, static_cast<unsigned>(std::lround(kHeight)) * n);
	if (canvas_ == nullptr)
	{
		canvas_ = std::make_unique<Canvas2D>(gfx, w, h);
		canvas_->LinkTechniques(rg);
		return;
	}
	if (canvas_->GetCanvasWidth() != w || canvas_->GetCanvasHeight() != h)
	{
		canvas_->Resize(w, h);
	}
}

void ModuleShopPanel::SetWorldCenter(DirectX::XMFLOAT3 center) noexcept
{
	if (canvas_ == nullptr)
	{
		return;
	}
	canvas_->SetPosition(center);
	canvas_->SetScale(DirectX::XMFLOAT3{ kWidth, kHeight, 1.0f });
}

void ModuleShopPanel::Submit() const
{
	if (canvas_ != nullptr)
	{
		canvas_->Submit(Chan::ui);
	}
}

void ModuleShopPanel::Rebuild(const IModuleNode* node, int price, bool sold, bool locked)
{
	if (canvas_ == nullptr)
	{
		return;
	}
	if (sold)
	{
		ModuleShopDraw::PaintTradeCardSold(*canvas_);
		return;
	}
	if (node == nullptr)
	{
		ModuleShopDraw::PaintTradeCardChrome(*canvas_, true, false);
		canvas_->NotifyPixelsChanged();
		return;
	}
	ModuleShopDraw::PaintTradeCardStock(*canvas_, *node, price, locked);
}
