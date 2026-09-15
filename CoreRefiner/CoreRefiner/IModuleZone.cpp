#include "IModuleZone.h"
#include "CanvasPixelDraw.h"
#include "Channels.h"
#include "Collision2D.h"
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

void IModuleZone::GetShellPixelSize_(unsigned& w, unsigned& h) const noexcept
{
	const BoundsWorld b = GetShellBoundsWorld();
	w = (std::max)(1u, static_cast<unsigned>(std::lround(b.half.x * 2.0f)));
	h = (std::max)(1u, static_cast<unsigned>(std::lround(b.half.y * 2.0f)));
}

void IModuleZone::EnsureShell_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	if (shell_ != nullptr)
	{
		RebuildShellIfNeeded_();
		return;
	}

	unsigned w = 1u;
	unsigned h = 1u;
	GetShellPixelSize_(w, h);
	shell_ = std::make_unique<Canvas2D>(gfx, w, h);
	PaintShell_();
	shell_->LinkTechniques(rg);
	SyncShellTransform_();
}

void IModuleZone::RebuildShellIfNeeded_()
{
	if (shell_ == nullptr)
	{
		return;
	}

	unsigned w = 1u;
	unsigned h = 1u;
	GetShellPixelSize_(w, h);
	if (shell_->GetCanvasWidth() == w && shell_->GetCanvasHeight() == h)
	{
		return;
	}

	shell_->Resize(w, h);
	PaintShell_();
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
	constexpr int kOuterThickness = 3;
	constexpr int kInnerThickness = 2;
	constexpr int kOuterInset = 1;
	constexpr int kInnerInset = 6;

	shell_->Clear(kBg);

	const int w = static_cast<int>(shell_->GetCanvasWidth());
	const int h = static_cast<int>(shell_->GetCanvasHeight());
	CanvasPixelDraw::DrawRectOutlineThick(
		*shell_,
		kOuterInset,
		kOuterInset,
		w - 1 - kOuterInset,
		h - 1 - kOuterInset,
		kOuterThickness,
		kOuter);
	CanvasPixelDraw::DrawRectOutlineThick(
		*shell_,
		kInnerInset,
		kInnerInset,
		w - 1 - kInnerInset,
		h - 1 - kInnerInset,
		kInnerThickness,
		kInner);
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
	RebuildShellIfNeeded_();
	SyncShellTransform_();
	SyncZoneTransforms_();
}

void IModuleZone::SubmitBackground()
{
	SubmitShell_();
	SubmitZoneBackground_();
}

DirectX::XMFLOAT2 IModuleZone::ContentLocalToWorld_(DirectX::XMFLOAT2 local) const noexcept
{
	const DirectX::XMFLOAT3 o = GetOrigin();
	return DirectX::XMFLOAT2{ o.x + local.x, o.y + local.y };
}

float IModuleZone::PickHitRadius_(const IModuleNode& node) const noexcept
{
	return node.GetVisualRadius();
}

IModuleNode* IModuleZone::PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept
{
	IModuleNode* best = nullptr;
	float bestDistSq = 1.0e9f;

	for (std::size_t i = 0; i < GetNodeCount(); ++i)
	{
		IModuleNode* node = GetNode(i);
		if (node == nullptr)
		{
			continue;
		}
		// 炼成停放开着 ghost：只能从格子里点，影子不拾取。占用仍走碰撞位。
		if (node->IsLayoutGhostActive())
		{
			continue;
		}
		const DirectX::XMFLOAT2 world = ContentLocalToWorld_(node->GetCollisionLocalPos());
		const Collider2D::CircleCollider hit{ world, PickHitRadius_(*node) };
		const Collider2D::PointCollider pt{ worldPos };
		if (!Collider2D::CollisionSystem::IsOverlap(hit, pt))
		{
			continue;
		}
		const float dx = worldPos.x - world.x;
		const float dy = worldPos.y - world.y;
		const float distSq = dx * dx + dy * dy;
		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			best = node;
		}
	}

	if (best != nullptr)
	{
		outDistSq = bestDistSq;
	}
	return best;
}