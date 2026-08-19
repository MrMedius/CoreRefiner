#include "IModuleNode.h"
#include "Channels.h"
#include "Colors.h"
#include "IconAtlas.h"
#include "RenderGraph.h"
#include "XMath.h"

#include <algorithm>

std::uint32_t IModuleNode::s_nextInstanceId_ = 0;

namespace
{
	void TintWhiteShapePixels(Canvas& c, const Color color)
	{
		const unsigned w = c.GetCanvasWidth();
		const unsigned h = c.GetCanvasHeight();
		for (unsigned y = 0u; y < h; ++y)
		{
			for (unsigned x = 0u; x < w; ++x)
			{
				const Color px = c.GetPixel(x, y);
				if (px.GetA() > 0u)
				{
					c.PutPixel(x, y, color);
				}
			}
		}
	}
}

Color IModuleNode::GetReadyFillColor() const noexcept
{
	if (isCore_)
	{
		return Color(255u, 210u, 60u, 255u);
	}
	return Color(120u, 200u, 255u, 255u);
}

void IModuleNode::InitVisual(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 zoneOrigin)
{
	zoneOrigin_ = zoneOrigin;

	icon_ = std::make_unique<Canvas2D>(gfx, kVisualSize, kVisualSize);
	icon_->Clear(Colors::None);
	IconAtlas::BlitIcon(
		*icon_,
		NodeIconAtlas::Get(GetModuleNodeLabel()),
		GetReadyFillColor());
	icon_->NotifyPixelsChanged();
	icon_->LinkTechniques(rg);

	mask_ = std::make_unique<Canvas2DSpriteUV>(gfx, kVisualSize, kVisualSize);
	mask_->Clear(Colors::None);
	IconAtlas::BlitIcon(
		*mask_,
		NodeIconAtlas::Get(GetModuleNodeLabel()),
		Color(0u, 0u, 0u, 160u));
	mask_->NotifyPixelsChanged();
	mask_->NotifyPixelsChanged();
	mask_->LinkTechniques(rg);
	mask_->SetUVOffset(0.0f, 0.0f);
	mask_->SetUVScale(1.0f, 0.0f);

	visualReady_ = true;

	SyncMaskUV_();
	ApplyVisualTransform_();
}

void IModuleNode::SetZoneOrigin(DirectX::XMFLOAT3 zoneOrigin) noexcept
{
	zoneOrigin_ = zoneOrigin;
	if (visualReady_)
	{
		ApplyVisualTransform_();
	}
}

void IModuleNode::BeginLayoutGhost(DirectX::XMFLOAT2 at) noexcept
{
	layoutGhostActive_ = true;
	layoutGhostLocalPos_ = at;
	if (mask_ != nullptr)
	{
		mask_->SetUVOffset(0.0f, 0.0f);
		mask_->SetUVScale(1.0f, 1.0f);
	}
	if (visualReady_)
	{
		ApplyVisualTransform_();
	}
}

void IModuleNode::EndLayoutGhost() noexcept
{
	layoutGhostActive_ = false;
	layoutGhostLocalPos_ = {};
	if (mask_ != nullptr && state_ != ModuleReadyState::Cooling)
	{
		mask_->SetUVScale(1.0f, 0.0f);
	}
	else
	{
		SyncMaskUV_();
	}
	if (visualReady_)
	{
		ApplyVisualTransform_();
	}
}

void IModuleNode::SyncVisual()
{
	if (!visualReady_)
	{
		return;
	}

	SyncMaskUV_();
	ApplyVisualTransform_();
}

void IModuleNode::SubmitVisual()
{
	if (!visualReady_)
	{
		return;
	}
	// Ghost mask under icon; cooldown mask over icon — mutually exclusive in normal flow.
	if (layoutGhostActive_ && mask_ != nullptr)
	{
		mask_->Submit(Chan::ui);
	}
	if (icon_ != nullptr)
	{
		icon_->Submit(Chan::ui);
	}
	if (mask_ == nullptr)
	{
		return;
	}
	const float remainRatio = GetRemainRatio_();
	if (state_ == ModuleReadyState::Cooling && remainRatio > 0.0f)
	{
		mask_->Submit(Chan::ui);
	}
}

void IModuleNode::ApplyVisualTransform_()
{
	if (icon_ == nullptr || mask_ == nullptr)
	{
		return;
	}

	const float iconSide = GetIconRadius() * 2.0f;
	const float maskSide = GetVisualRadius() * 2.0f;

	const DirectX::XMFLOAT3 iconPos = (
		V(zoneOrigin_) + Vec3{ localPos_.x, localPos_.y, 0.0f }
	).ToFloat3();
	icon_->SetPosition(iconPos);
	icon_->SetScale(DirectX::XMFLOAT3{ iconSide, iconSide, 1.0f });

	const DirectX::XMFLOAT2 maskLocal = layoutGhostActive_ ? layoutGhostLocalPos_ : localPos_;
	const DirectX::XMFLOAT3 maskPos = (
		V(zoneOrigin_) + Vec3{ maskLocal.x, maskLocal.y, 0.0f }
	).ToFloat3();
	mask_->SetPosition(maskPos);
	mask_->SetScale(DirectX::XMFLOAT3{ maskSide, maskSide, 1.0f });
}

float IModuleNode::GetRemainRatio_() const noexcept
{
	if (state_ != ModuleReadyState::Cooling)
	{
		return 0.0f;
	}
	const float duration = std::max(cooldownDuration_, 1.0e-6f);
	return std::clamp(cooldownRemaining_ / duration, 0.0f, 1.0f);
}

void IModuleNode::SyncMaskUV_()
{
	if (mask_ == nullptr)
	{
		return;
	}
	// Layout ghost owns full-reveal UV until EndLayoutGhost.
	if (layoutGhostActive_)
	{
		mask_->SetUVScale(1.0f, 1.0f);
		return;
	}
	mask_->SetUVScale(1.0f, GetRemainRatio_());
}
