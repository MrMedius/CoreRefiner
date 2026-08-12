#include "ModuleField.h"
#include "Channels.h"
#include "Collision2D.h"
#include "RenderGraph.h"

#include <algorithm>
#include <cmath>

void ModuleField::InitAllVisuals(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 fieldOrigin)
{
	origin_ = fieldOrigin;
	if (canvas_ == nullptr)
	{
		constexpr float side = ModuleFieldCanvas::kDefaultFieldSide;
		canvas_ = std::make_unique<ModuleFieldCanvas>(gfx, 300u, 300u);
		canvas_->SetScale(DirectX::XMFLOAT3{ side, side, 1.0f });
		canvas_->LinkTechniques(rg);
	}
	canvas_->SetPosition(origin_);

	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->InitVisual(gfx, rg, origin_);
		}
	}
}

void ModuleField::SetFieldOrigin(DirectX::XMFLOAT3 fieldOrigin) noexcept
{
	origin_ = fieldOrigin;
	if (canvas_ != nullptr)
	{
		canvas_->SetPosition(origin_);
	}
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->SetFieldOrigin(origin_);
		}
	}
}

void ModuleField::SyncAllVisuals()
{
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->SyncVisual();
		}
	}
}

void ModuleField::SubmitBackground()
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

void ModuleField::SubmitAllVisuals()
{
	SubmitBackground();
	SubmitNodes();
}

bool ModuleField::ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept
{
	const float r = (std::max)(radius, 0.0f);
	const float usable = kHalfExtent - r;
	if (usable < 0.0f)
	{
		return false;
	}
	const float lx = worldCenter.x - origin_.x;
	const float ly = worldCenter.y - origin_.y;
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
	const IFieldNode& self,
	DirectX::XMFLOAT2 fieldLocal) const noexcept
{
	const Collider2D::CircleCollider moving{ fieldLocal, self.GetHitRadius() };
	for (const auto& n : nodes_)
	{
		const IFieldNode* other = n.get();
		if (other == nullptr || other == &self)
		{
			continue;
		}
		const Collider2D::CircleCollider solid{ other->GetLocalPos(), other->GetHitRadius() };
		if (Collider2D::CollisionSystem::IsOverlap(moving, solid))
		{
			return true;
		}
	}
	return false;
}

IFieldNode* ModuleField::PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept
{
	IFieldNode* best = nullptr;
	float bestDistSq = 1.0e9f;

	for (auto& n : nodes_)
	{
		IFieldNode* node = n.get();
		if (node == nullptr)
		{
			continue;
		}
		const DirectX::XMFLOAT2 local = node->GetLocalPos();
		const DirectX::XMFLOAT2 world{
			origin_.x + local.x,
			origin_.y + local.y
		};
		const Collider2D::CircleCollider hit{ world, node->GetHitRadius() };
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
