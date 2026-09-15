#include "ModuleField.h"
#include "Channels.h"
#include "Collision2D.h"
#include "RenderGraph.h"

#include <algorithm>
#include <cmath>

void ModuleField::InitZoneVisuals_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	if (canvas_ == nullptr)
	{
		canvas_ = std::make_unique<ModuleFieldCanvas>(gfx, 300u, 300u);
		canvas_->LinkTechniques(rg);
	}
	canvas_->SetPosition(origin_);
	ApplyDisplayScale_();

	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->InitVisual(gfx, rg, origin_);
			n->SetZoneVisualScale(DisplayScale_());
		}
	}
}

void ModuleField::SetOrigin(DirectX::XMFLOAT3 origin) noexcept
{
	origin_ = origin;
	if (canvas_ != nullptr)
	{
		canvas_->SetPosition(origin_);
	}
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->SetZoneOrigin(origin_);
		}
	}
}

void ModuleField::SetVisualScale(float scale) noexcept
{
	visualScale_ = (scale > 0.0f) ? scale : 1.0f;
	ApplyDisplayScale_();
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->SetZoneVisualScale(visualScale_);
		}
	}
	SyncAllVisuals();
}

void ModuleField::ApplyDisplayScale_() noexcept
{
	if (canvas_ == nullptr)
	{
		return;
	}
	const float side = ModuleFieldCanvas::kDefaultFieldSide * DisplayScale_();
	canvas_->SetScale(DirectX::XMFLOAT3{ side, side, 1.0f });
}

float ModuleField::DisplayScale_() const noexcept
{
	return (visualScale_ > 0.0f) ? visualScale_ : 1.0f;
}

DirectX::XMFLOAT2 ModuleField::WorldToLocal_(DirectX::XMFLOAT2 world) const noexcept
{
	const float s = DisplayScale_();
	return DirectX::XMFLOAT2{
		(world.x - origin_.x) / s,
		(world.y - origin_.y) / s
	};
}

DirectX::XMFLOAT2 ModuleField::ContentLocalToWorld_(DirectX::XMFLOAT2 local) const noexcept
{
	const float s = DisplayScale_();
	return DirectX::XMFLOAT2{
		origin_.x + local.x * s,
		origin_.y + local.y * s
	};
}

float ModuleField::PickHitRadius_(const IModuleNode& node) const noexcept
{
	return node.GetHitRadius() * DisplayScale_();
}

bool ModuleField::TryAcceptDrop(
	std::unique_ptr<IModuleNode>& node,
	DirectX::XMFLOAT2 localPos)
{
	if (node == nullptr)
	{
		return false;
	}

	IModuleNode* raw = node.get();
	raw->SetLocalPos(localPos);
	raw->SetZoneOrigin(origin_);
	raw->SetZoneVisualScale(DisplayScale_());
	nodes_.push_back(std::move(node));
	raw->SyncVisual();
	return true;
}

DropResult ModuleField::EvalDrop(
	const IModuleNode& node,
	DirectX::XMFLOAT2 worldPos,
	ZoneId from) const noexcept
{
	(void)from;
	DropResult result{};
	const float radius = node.GetHitRadius();
	if (!ContainsCircle(worldPos, radius))
	{
		result.verdict = DropVerdict::OutOfBounds;
		return result;
	}

	DirectX::XMFLOAT2 local = WorldToLocal_(worldPos);
	local = ClampLocalForRadius(local, radius);
	result.localPos = local;
	if (WouldOverlap(node, local))
	{
		result.verdict = DropVerdict::Blocked;
		return result;
	}

	result.verdict = DropVerdict::Accept;
	return result;
}

void ModuleField::SyncZoneTransforms_()
{
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->SyncVisual();
		}
	}
}

void ModuleField::SubmitZoneBackground_()
{
	if (canvas_ != nullptr)
	{
		canvas_->Submit(Chan::ui);
	}
}

void ModuleField::SubmitNodes()
{
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->SubmitVisual();
		}
	}
}

ModuleField::BoundsWorld ModuleField::GetBoundsWorld() const noexcept
{
	BoundsWorld b{};
	const float s = DisplayScale_();
	b.center = DirectX::XMFLOAT2{ origin_.x, origin_.y };
	b.half = DirectX::XMFLOAT2{ kHalfExtent * s, kHalfExtent * s };
	return b;
}

bool ModuleField::ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept
{
	const float r = (std::max)(radius, 0.0f);
	const float usable = kHalfExtent - r;
	if (usable < 0.0f)
	{
		return false;
	}
	const DirectX::XMFLOAT2 local = WorldToLocal_(worldCenter);
	const float lx = local.x;
	const float ly = local.y;
	return std::fabs(lx) <= usable && std::fabs(ly) <= usable;
}

DirectX::XMFLOAT2 ModuleField::ClampLocalForRadius(
	DirectX::XMFLOAT2 localPos,
	float hitRadius) const noexcept
{
	const float half = (std::max)(kHalfExtent - hitRadius, 0.0f);
	const Collider2D::BoxCollider bounds = Collider2D::BoxCollider::MakeCenteredSquare(half);
	return Collider2D::ClampPointToBox(localPos, bounds);
}

bool ModuleField::WouldOverlap(
	const IModuleNode& self,
	DirectX::XMFLOAT2 fieldLocal) const noexcept
{
	const Collider2D::CircleCollider moving{ fieldLocal, self.GetHitRadius() };
	for (const auto& n : nodes_)
	{
		const IModuleNode* other = n.get();
		if (other == nullptr || other == &self)
		{
			continue;
		}
		// 停进炼成后图标飞走，占用仍钉在残影上。
		const Collider2D::CircleCollider solid{ other->GetCollisionLocalPos(), other->GetHitRadius() };
		if (Collider2D::CollisionSystem::IsOverlap(moving, solid))
		{
			return true;
		}
	}
	return false;
}