#include "ModuleWarehouse.h"
#include "CanvasPixelDraw.h"
#include "Channels.h"
#include "Collision2D.h"
#include "Colors.h"
#include "RenderGraph.h"

#include <cmath>

float ModuleWarehouse::HalfSpanX_() noexcept
{
	return (static_cast<float>(kColumns - 1) * 0.5f) * kSlotPitch;
}

float ModuleWarehouse::HalfSpanY_() noexcept
{
	return (static_cast<float>(kMaxRows - 1) * 0.5f) * kSlotPitch;
}

DirectX::XMFLOAT2 ModuleWarehouse::SlotLocalPos_(std::size_t index) noexcept
{
	const int col = static_cast<int>(index % static_cast<std::size_t>(kColumns));
	const int row = static_cast<int>(index / static_cast<std::size_t>(kColumns));
	const float x = (static_cast<float>(col) - (static_cast<float>(kColumns - 1) * 0.5f)) * kSlotPitch;
	const float y = static_cast<float>(row) * kSlotPitch;
	return DirectX::XMFLOAT2{ x, y };
}

std::size_t ModuleWarehouse::FindFirstEmptySlot_() const noexcept
{
	for (std::size_t i = 0; i < enabledSlots_; ++i)
	{
		if (nodes_[i] == nullptr)
		{
			return i;
		}
	}
	return kMaxSlots;
}

std::size_t ModuleWarehouse::SlotIndexAtLocal_(DirectX::XMFLOAT2 localPos) const noexcept
{
	const float colF = localPos.x / kSlotPitch + static_cast<float>(kColumns - 1) * 0.5f;
	const float rowF = localPos.y / kSlotPitch;
	const int col = static_cast<int>(std::lround(colF));
	const int row = static_cast<int>(std::lround(rowF));
	if (col < 0 || col >= kColumns || row < 0 || row >= kMaxRows)
	{
		return kMaxSlots;
	}
	const std::size_t index = static_cast<std::size_t>(row) * static_cast<std::size_t>(kColumns)
		+ static_cast<std::size_t>(col);
	const DirectX::XMFLOAT2 center = SlotLocalPos_(index);
	if (std::fabs(localPos.x - center.x) > kCellHalfExtent
		|| std::fabs(localPos.y - center.y) > kCellHalfExtent)
	{
		return kMaxSlots;
	}
	return index;
}

std::size_t ModuleWarehouse::SlotIndexAt_(DirectX::XMFLOAT2 worldPos) const noexcept
{
	return SlotIndexAtLocal_(DirectX::XMFLOAT2{
		worldPos.x - warehouseOrigin_.x,
		worldPos.y - warehouseOrigin_.y
	});
}

void ModuleWarehouse::SetEnabledSlotCount(std::size_t count) noexcept
{
	enabledSlots_ = (count > kMaxSlots) ? kMaxSlots : count;
	if (panel_ != nullptr)
	{
		PaintPanel_();
	}
}

ModuleWarehouse::BoundsWorld ModuleWarehouse::GetBoundsWorld() const noexcept
{
	// Slot centers span: x in [-(cols-1)/2, +(cols-1)/2]*pitch, y in [0, (rows-1)*pitch].
	const float halfSpanX = HalfSpanX_();
	const float halfSpanY = HalfSpanY_();
	const float localCenterY = halfSpanY;

	BoundsWorld b{};
	b.center = DirectX::XMFLOAT2{
		warehouseOrigin_.x,
		warehouseOrigin_.y + localCenterY
	};
	b.half = DirectX::XMFLOAT2{
		halfSpanX + kBoundsPad,
		halfSpanY + kBoundsPad
	};
	return b;
}

bool ModuleWarehouse::ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept
{
	(void)radius;
	const BoundsWorld b = GetBoundsWorld();
	const float minX = b.center.x - b.half.x;
	const float maxX = b.center.x + b.half.x;
	const float minY = b.center.y - b.half.y;
	const float maxY = b.center.y + b.half.y;
	return worldCenter.x >= minX && worldCenter.x <= maxX
		&& worldCenter.y >= minY && worldCenter.y <= maxY;
}

void ModuleWarehouse::SetOrigin(DirectX::XMFLOAT3 origin) noexcept
{
	warehouseOrigin_ = origin;
	RelayoutSlots();
	SyncPanelTransform_();
}

void ModuleWarehouse::RelayoutSlots()
{
	for (std::size_t i = 0; i < kMaxSlots; ++i)
	{
		IModuleNode* node = nodes_[i].get();
		if (node == nullptr)
		{
			continue;
		}
		node->SetLocalPos(SlotLocalPos_(i));
		node->SetZoneOrigin(warehouseOrigin_);
		node->SetVisualRadiusOverride(kStoredVisualRadius);
		node->SyncVisual();
	}
}

bool ModuleWarehouse::TryAcceptDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos)
{
	if (node == nullptr || node->IsCore())
	{
		return false;
	}
	const std::size_t slot = SlotIndexAtLocal_(localPos);
	if (slot >= enabledSlots_ || nodes_[slot] != nullptr)
	{
		return false;
	}
	nodes_[slot] = std::move(node);
	RelayoutSlots();
	return true;
}

DropResult ModuleWarehouse::EvalDrop(const IModuleNode& node, DirectX::XMFLOAT2 worldPos, ZoneId from) const noexcept
{
	DropResult result{};
	if (!ContainsCircle(worldPos, node.GetHitRadius()))
	{
		result.verdict = DropVerdict::OutOfBounds;
		return result;
	}

	if (from != ZoneId::Warehouse && node.IsCore())
	{
		result.verdict = DropVerdict::Forbidden;
		return result;
	}

	const std::size_t slot = SlotIndexAt_(worldPos);
	if (slot >= kMaxSlots)
	{
		result.verdict = DropVerdict::OutOfBounds;
		return result;
	}
	if (slot >= enabledSlots_)
	{
		result.verdict = DropVerdict::NoSpace;
		return result;
	}
	if (nodes_[slot] != nullptr && nodes_[slot].get() != &node)
	{
		result.verdict = DropVerdict::Blocked;
		return result;
	}

	result.verdict = DropVerdict::Accept;
	result.localPos = SlotLocalPos_(slot);
	return result;
}

void ModuleWarehouse::OnSameZoneMove(IModuleNode& node, DirectX::XMFLOAT2 localPos)
{
	const std::size_t dest = SlotIndexAtLocal_(localPos);
	if (dest >= enabledSlots_)
	{
		return;
	}

	std::size_t src = kMaxSlots;
	for (std::size_t i = 0; i < kMaxSlots; ++i)
	{
		if (nodes_[i].get() == &node)
		{
			src = i;
			break;
		}
	}
	if (src >= kMaxSlots || src == dest || nodes_[dest] != nullptr)
	{
		return;
	}

	nodes_[dest] = std::move(nodes_[src]);
	RelayoutSlots();
}

std::unique_ptr<IModuleNode> ModuleWarehouse::TakeNode(std::size_t index)
{
	if (index >= kMaxSlots)
	{
		return nullptr;
	}
	std::unique_ptr<IModuleNode> out = std::move(nodes_[index]);
	if (out != nullptr)
	{
		out->ClearVisualRadiusOverride();
	}
	RelayoutSlots();
	return out;
}

std::unique_ptr<IModuleNode> ModuleWarehouse::TakeNode(IModuleNode* node)
{
	if (node == nullptr)
	{
		return nullptr;
	}
	for (std::size_t i = 0; i < kMaxSlots; ++i)
	{
		if (nodes_[i].get() == node)
		{
			return TakeNode(i);
		}
	}
	return nullptr;
}

void ModuleWarehouse::EnsurePanelVisual_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	if (panel_ != nullptr)
	{
		return;
	}

	const float halfX = HalfSpanX_() + kBoundsPad;
	const float halfY = HalfSpanY_() + kBoundsPad;
	const unsigned w = static_cast<unsigned>(std::lround(halfX * 2.0f));
	const unsigned h = static_cast<unsigned>(std::lround(halfY * 2.0f));

	panel_ = std::make_unique<Canvas2D>(gfx, w, h);
	PaintPanel_();
	panel_->LinkTechniques(rg);
	SyncPanelTransform_();
}

void ModuleWarehouse::PaintPanel_()
{
	if (panel_ == nullptr)
	{
		return;
	}

	constexpr Color kBg{ 36u, 48u, 68u, 150u };
	constexpr Color kGrid{ 170u, 190u, 210u, 110u };
	constexpr Color kDisabledX{ 90u, 100u, 120u, 180u };

	panel_->Clear(kBg);

	const float halfX = HalfSpanX_() + kBoundsPad;
	const float halfY = HalfSpanY_() + kBoundsPad;
	const float localCenterY = HalfSpanY_();

	for (std::size_t i = 0; i < kMaxSlots; ++i)
	{
		const DirectX::XMFLOAT2 slot = SlotLocalPos_(i);
		// Warehouse-local → panel pixel (bounds center is local (0, localCenterY)).
		const float cx = slot.x + halfX;
		const float cy = slot.y - localCenterY + halfY;
		const int x0 = static_cast<int>(std::lround(cx - kCellHalfExtent));
		const int y0 = static_cast<int>(std::lround(cy - kCellHalfExtent));
		const int x1 = static_cast<int>(std::lround(cx + kCellHalfExtent));
		const int y1 = static_cast<int>(std::lround(cy + kCellHalfExtent));
		CanvasPixelDraw::DrawRectOutline(*panel_, x0, y0, x1, y1, kGrid);
		if (i >= enabledSlots_)
		{
			constexpr int kInset = 6;
			CanvasPixelDraw::DrawLine(*panel_, x0 + kInset, y0 + kInset, x1 - kInset, y1 - kInset, kDisabledX);
			CanvasPixelDraw::DrawLine(*panel_, x1 - kInset, y0 + kInset, x0 + kInset, y1 - kInset, kDisabledX);
		}
	}

	panel_->NotifyPixelsChanged();
}

void ModuleWarehouse::SyncPanelTransform_() noexcept
{
	if (panel_ == nullptr)
	{
		return;
	}

	const BoundsWorld b = GetBoundsWorld();
	panel_->SetPosition(DirectX::XMFLOAT3{ b.center.x, b.center.y, 0.0f });
	panel_->SetScale(DirectX::XMFLOAT3{
		b.half.x * 2.0f,
		b.half.y * 2.0f,
		1.0f
	});
}

void ModuleWarehouse::InitZoneVisuals_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	EnsurePanelVisual_(gfx, rg);
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->InitVisual(gfx, rg, warehouseOrigin_);
		}
	}
	RelayoutSlots();
	SyncPanelTransform_();
}

void ModuleWarehouse::SyncZoneTransforms_()
{
	SyncPanelTransform_();
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->SyncVisual();
		}
	}
}

void ModuleWarehouse::SubmitZoneBackground_()
{
	if (panel_ != nullptr)
	{
		panel_->Submit(Chan::ui);
	}
}

void ModuleWarehouse::SubmitNodes()
{
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->SubmitVisual();
		}
	}
}

IModuleNode* ModuleWarehouse::PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept
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
			warehouseOrigin_.x + local.x,
			warehouseOrigin_.y + local.y
		};
		const Collider2D::CircleCollider hit{ world, node->GetVisualRadius() };
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