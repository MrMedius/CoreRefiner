#include "IModuleZone.h"
#include "CanvasPixelDraw.h"
#include "Channels.h"
#include "Colors.h"
#include "Graphics.h"
#include "RenderGraph.h"

#include <algorithm>
#include <cmath>

IModuleZone::BoundsWorld IModuleZone::GetShellBoundsWorld() const noexcept
{
	BoundsWorld b = GetBoundsWorld();
	b.half.x += kShellPad;
	b.half.y += kShellPad;
	return b;
}

void IModuleZone::EnsureShell_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	if (shell_ != nullptr)
	{
		return;
	}

	const BoundsWorld b = GetShellBoundsWorld();
	const unsigned w = static_cast<unsigned>(std::lround(b.half.x * 2.0f));
	const unsigned h = static_cast<unsigned>(std::lround(b.half.y * 2.0f));
	shell_ = std::make_unique<Canvas2D>(gfx, (std::max)(1u, w), (std::max)(1u, h));
	PaintShell_();
	shell_->LinkTechniques(rg);
	SyncShellTransform_();
}

void IModuleZone::PaintShell_()
{
	if (shell_ == nullptr)
	{
		return;
	}

	constexpr Color kBg{ 18u, 22u, 32u, 200u };
	constexpr Color kOuter{ 210u, 220u, 235u, 180u };
	constexpr Color kInner{ 150u, 165u, 185u, 120u };

	shell_->Clear(kBg);

	const int w = static_cast<int>(shell_->GetCanvasWidth());
	const int h = static_cast<int>(shell_->GetCanvasHeight());
	CanvasPixelDraw::DrawRectOutline(*shell_, 1, 1, w - 2, h - 2, kOuter);
	CanvasPixelDraw::DrawRectOutline(*shell_, 4, 4, w - 5, h - 5, kInner);
	shell_->NotifyPixelsChanged();
}

void IModuleZone::SyncShellTransform_() noexcept
{
	if (shell_ == nullptr)
	{
		return;
	}

	const BoundsWorld b = GetShellBoundsWorld();
	shell_->SetPosition(DirectX::XMFLOAT3{ b.center.x, b.center.y, 0.0f });
	shell_->SetScale(DirectX::XMFLOAT3{
		b.half.x * 2.0f,
		b.half.y * 2.0f,
		1.0f
	});
}

void IModuleZone::SubmitShell_()
{
	if (shell_ != nullptr)
	{
		shell_->Submit(Chan::ui);
	}
}

void IModuleZone::InitAllVisuals(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 origin)
{
	SetOrigin(origin);
	EnsureShell_(gfx, rg);
	SyncShellTransform_();
	InitZoneVisuals_(gfx, rg);
}

void IModuleZone::SyncAllVisuals()
{
	SyncShellTransform_();
	SyncZoneTransforms_();
}

void IModuleZone::SubmitBackground()
{
	SubmitShell_();
	SubmitZoneBackground_();
}