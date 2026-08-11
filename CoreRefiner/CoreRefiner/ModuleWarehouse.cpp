#include "ModuleWarehouse.h"

#include <algorithm>

DirectX::XMFLOAT2 ModuleWarehouse::SlotLocalPos_(std::size_t index) noexcept
{
	const int col = static_cast<int>(index % static_cast<std::size_t>(kColumns));
	const int row = static_cast<int>(index / static_cast<std::size_t>(kColumns));
	const float x = (static_cast<float>(col) - (static_cast<float>(kColumns - 1) * 0.5f)) * kSlotPitch;
	const float y = static_cast<float>(row) * kSlotPitch;
	return DirectX::XMFLOAT2{ x, y };
}

ModuleWarehouse::BoundsWorld ModuleWarehouse::GetBoundsWorld() const noexcept
{
	// Slot centers span: x in [-(cols-1)/2, +(cols-1)/2]*pitch, y in [0, (rows-1)*pitch].
	const float halfSpanX = (static_cast<float>(kColumns - 1) * 0.5f) * kSlotPitch;
	const float halfSpanY = (static_cast<float>(kMaxRows - 1) * 0.5f) * kSlotPitch;
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
	const BoundsWorld b = GetBoundsWorld();
	const float r = (std::max)(radius, 0.0f);
	const float minX = b.center.x - b.half.x + r;
	const float maxX = b.center.x + b.half.x - r;
	const float minY = b.center.y - b.half.y + r;
	const float maxY = b.center.y + b.half.y - r;
	if (minX > maxX || minY > maxY)
	{
		return false;
	}
	return worldCenter.x >= minX && worldCenter.x <= maxX
		&& worldCenter.y >= minY && worldCenter.y <= maxY;
}

void ModuleWarehouse::SetOrigin(DirectX::XMFLOAT3 origin) noexcept
{
	warehouseOrigin_ = origin;
	RelayoutSlots();
}

void ModuleWarehouse::RelayoutSlots()
{
	for (std::size_t i = 0; i < nodes_.size(); ++i)
	{
		IFieldNode* node = nodes_[i].get();
		if (node == nullptr)
		{
			continue;
		}
		node->SetLocalPos(SlotLocalPos_(i));
		node->SetFieldOrigin(warehouseOrigin_);
		node->SyncVisual();
	}
}

IFieldNode* ModuleWarehouse::TryAdopt(std::unique_ptr<IFieldNode>& node)
{
	if (node == nullptr || node->IsCore() || IsFull())
	{
		return nullptr;
	}
	IFieldNode* raw = node.get();
	nodes_.push_back(std::move(node));
	RelayoutSlots();
	return raw;
}

std::unique_ptr<IFieldNode> ModuleWarehouse::TakeNode(std::size_t index)
{
	if (index >= nodes_.size())
	{
		return nullptr;
	}
	std::unique_ptr<IFieldNode> out = std::move(nodes_[index]);
	nodes_.erase(nodes_.begin() + static_cast<std::ptrdiff_t>(index));
	RelayoutSlots();
	return out;
}

std::unique_ptr<IFieldNode> ModuleWarehouse::TakeNode(IFieldNode* node)
{
	if (node == nullptr)
	{
		return nullptr;
	}
	for (std::size_t i = 0; i < nodes_.size(); ++i)
	{
		if (nodes_[i].get() == node)
		{
			return TakeNode(i);
		}
	}
	return nullptr;
}

void ModuleWarehouse::InitAllVisuals(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 origin)
{
	warehouseOrigin_ = origin;
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->InitVisual(gfx, rg, warehouseOrigin_);
		}
	}
	RelayoutSlots();
}

void ModuleWarehouse::SyncAllVisuals()
{
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->SyncVisual();
		}
	}
}

void ModuleWarehouse::SubmitAllVisuals()
{
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->SubmitVisual();
		}
	}
}
