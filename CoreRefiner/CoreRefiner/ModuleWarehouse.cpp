#include "ModuleWarehouse.h"

DirectX::XMFLOAT2 ModuleWarehouse::SlotLocalPos_(std::size_t index) noexcept
{
	const int col = static_cast<int>(index % static_cast<std::size_t>(kColumns));
	const int row = static_cast<int>(index / static_cast<std::size_t>(kColumns));
	const float x = (static_cast<float>(col) - (static_cast<float>(kColumns - 1) * 0.5f)) * kSlotPitch;
	const float y = static_cast<float>(row) * kSlotPitch;
	return DirectX::XMFLOAT2{ x, y };
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
	if (node == nullptr || node->IsCore())
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
