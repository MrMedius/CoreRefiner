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

	DirectX::XMFLOAT2 local{
		worldPos.x - origin_.x,
		worldPos.y - origin_.y
	};
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

void ModuleField::SubmitAllVisuals()
{
	SubmitBackground();
	SubmitNodes();
}

ModuleField::BoundsWorld ModuleField::GetBoundsWorld() const noexcept
{
	BoundsWorld b{};
	b.center = DirectX::XMFLOAT2{ origin_.x, origin_.y };
	b.half = DirectX::XMFLOAT2{ kHalfExtent, kHalfExtent };
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
		const Collider2D::CircleCollider solid{ other->GetLocalPos(), other->GetHitRadius() };
		if (Collider2D::CollisionSystem::IsOverlap(moving, solid))
		{
			return true;
		}
	}
	return false;
}

IModuleNode* ModuleField::PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept
{
	IModuleNode* best = nullptr;
	float bestDistSq = 1.0e9f;

	for (auto& n : nodes_)
	{
		IModuleNode* node = n.get();
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
