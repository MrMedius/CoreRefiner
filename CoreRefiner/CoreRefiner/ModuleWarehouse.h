#pragma once
#include "IFieldNode.h"

#include <cstddef>
#include <DirectXMath.h>
#include <memory>
#include <utility>
#include <vector>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

class ModuleWarehouse
{
public:
	static constexpr int kColumns = 3;
	static constexpr float kSlotPitch = 48.0f;

	ModuleWarehouse() = default;
	~ModuleWarehouse() = default;

	ModuleWarehouse(const ModuleWarehouse&) = delete;
	ModuleWarehouse& operator=(const ModuleWarehouse&) = delete;

	template <typename T, typename... Args>
	T* AddNode(Args&&... args)
	{
		auto node = std::make_unique<T>(std::forward<Args>(args)...);
		if (node->IsCore())
		{
			return nullptr;
		}
		T* raw = node.get();
		nodes_.push_back(std::move(node));
		RelayoutSlots();
		return raw;
	}

	[[nodiscard]] std::size_t GetNodeCount() const noexcept { return nodes_.size(); }

	[[nodiscard]] IFieldNode* GetNode(std::size_t index) const noexcept
	{
		return (index < nodes_.size()) ? nodes_[index].get() : nullptr;
	}

	[[nodiscard]] DirectX::XMFLOAT3 GetOrigin() const noexcept { return warehouseOrigin_; }

	void SetOrigin(DirectX::XMFLOAT3 origin) noexcept;

	void RelayoutSlots();

	[[nodiscard]] IFieldNode* TryAdopt(std::unique_ptr<IFieldNode>& node);

	[[nodiscard]] std::unique_ptr<IFieldNode> TakeNode(std::size_t index);
	[[nodiscard]] std::unique_ptr<IFieldNode> TakeNode(IFieldNode* node);

	void InitAllVisuals(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 origin);
	void SyncAllVisuals();
	void SubmitAllVisuals();

	template <typename Fn>
	void ForEach(Fn&& fn)
	{
		for (auto& n : nodes_)
		{
			if (n != nullptr)
			{
				fn(*n);
			}
		}
	}

	template <typename Fn>
	void ForEach(Fn&& fn) const
	{
		for (const auto& n : nodes_)
		{
			if (n != nullptr)
			{
				fn(*n);
			}
		}
	}

private:
	[[nodiscard]] static DirectX::XMFLOAT2 SlotLocalPos_(std::size_t index) noexcept;

	std::vector<std::unique_ptr<IFieldNode>> nodes_;
	DirectX::XMFLOAT3 warehouseOrigin_{ 0.0f, 0.0f, 0.0f };
};
