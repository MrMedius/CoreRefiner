#include "IFieldNode.h"
#include "Channels.h"
#include "Colors.h"
#include "FieldIconAtlas.h"
#include "RenderGraph.h"
#include "XMath.h"

#include <algorithm>

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

Color IFieldNode::GetReadyFillColor() const noexcept
{
	if (isCore_)
	{
		return Color(255u, 210u, 60u, 255u);
	}
	return Color(120u, 200u, 255u, 255u);
}

void IFieldNode::InitVisual(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 fieldOrigin)
{
	fieldOrigin_ = fieldOrigin;

	icon_ = std::make_unique<Canvas2D>(gfx, kVisualSize, kVisualSize);
	icon_->Clear(Colors::None);
	FieldIconAtlas::BlitFieldIcon(
		*icon_,
		FieldIconAtlas::GetFieldIcon(GetAttackNodeLabel()),
		GetReadyFillColor());
	icon_->NotifyPixelsChanged();
	icon_->LinkTechniques(rg);

	mask_ = std::make_unique<Canvas2DSpriteUV>(gfx, kVisualSize, kVisualSize);
	mask_->Clear(Colors::None);
	FieldIconAtlas::BlitFieldIcon(
		*mask_,
		FieldIconAtlas::GetFieldIcon(GetAttackNodeLabel()),
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

void IFieldNode::SyncVisual()
{
	if (!visualReady_)
	{
		return;
	}

	SyncMaskUV_();
	ApplyVisualTransform_();
}

void IFieldNode::SubmitVisual()
{
	if (!visualReady_)
	{
		return;
	}
	if (icon_ != nullptr)
	{
		icon_->Submit(Chan::ui);
	}
	const float remainRatio = GetRemainRatio_();
	if (mask_ != nullptr
		&& state_ == ModuleReadyState::Cooling
		&& remainRatio > 0.0f)
	{
		mask_->Submit(Chan::ui);
	}
}

void IFieldNode::ApplyVisualTransform_()
{
	if (icon_ == nullptr || mask_ == nullptr)
	{
		return;
	}

	const DirectX::XMFLOAT3 pos = (
		V(fieldOrigin_) + Vec3{ localPos_.x, localPos_.y, 0.0f }
	).ToFloat3();
	const float side = hitRadius_ * 2.0f;
	const DirectX::XMFLOAT3 scale{ side, side, 1.0f };

	icon_->SetPosition(pos);
	icon_->SetScale(scale);
	mask_->SetPosition(pos);
	mask_->SetScale(scale);
}

float IFieldNode::GetRemainRatio_() const noexcept
{
	if (state_ != ModuleReadyState::Cooling)
	{
		return 0.0f;
	}
	const float duration = std::max(cooldownDuration_, 1.0e-6f);
	return std::clamp(cooldownRemaining_ / duration, 0.0f, 1.0f);
}

void IFieldNode::SyncMaskUV_()
{
	if (mask_ == nullptr)
	{
		return;
	}
	mask_->SetUVScale(1.0f, GetRemainRatio_());
}
