#pragma once
#include "Canvas2D.h"
#include "IModuleNode.h"
#include "IModuleZone.h"

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

class ModuleWarehouse : public IModuleZone
{
public:
	static constexpr int kColumns = 3;
	static constexpr int kMaxRows = 4;
	static constexpr std::size_t kMaxSlots = static_cast<std::size_t>(kColumns) * static_cast<std::size_t>(kMaxRows);
	static constexpr float kSlotPitch = 48.0f;
	static constexpr float kBoundsPad = 20.0f;

	struct BoundsWorld
	{
		DirectX::XMFLOAT2 center{ 0.0f, 0.0f };
		DirectX::XMFLOAT2 half{ 0.0f, 0.0f };
	};

	ModuleWarehouse() = default;
	~ModuleWarehouse() override = default;

	ModuleWarehouse(const ModuleWarehouse&) = delete;
	ModuleWarehouse& operator=(const ModuleWarehouse&) = delete;

	template <typename T, typename... Args>
	T* AddNode(Args&&... args)
	{
		if (IsFull())
		{
			return nullptr;
		}
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

	[[nodiscard]] ZoneId GetZoneId() const noexcept override { return ZoneId::Warehouse; }

	[[nodiscard]] std::size_t GetNodeCount() const noexcept override { return nodes_.size(); }

	[[nodiscard]] bool IsFull() const noexcept { return nodes_.size() >= kMaxSlots; }

	[[nodiscard]] bool HasFreeSlot() const noexcept { return !IsFull(); }

	[[nodiscard]] IModuleNode* GetNode(std::size_t index) const noexcept override
	{
		return (index < nodes_.size()) ? nodes_[index].get() : nullptr;
	}

	[[nodiscard]] DirectX::XMFLOAT3 GetOrigin() const noexcept override { return warehouseOrigin_; }

	[[nodiscard]] BoundsWorld GetBoundsWorld() const noexcept;

	[[nodiscard]] bool ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept override;

	void SetOrigin(DirectX::XMFLOAT3 origin) noexcept override;

	void RelayoutSlots();

	[[nodiscard]] bool TryAcceptDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos) override;

	[[nodiscard]] std::unique_ptr<IModuleNode> TakeNode(std::size_t index);
	[[nodiscard]] std::unique_ptr<IModuleNode> TakeNode(IModuleNode* node) override;

	void InitAllVisuals(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 origin) override;
	void SyncAllVisuals() override;
	void SubmitBackground() override;
	void SubmitNodes() override;
	void SubmitAllVisuals();

	void ClearLayoutGhosts() override
	{
		ForEach([](IModuleNode& node)
		{
			if (node.IsLayoutGhostActive())
			{
				node.EndLayoutGhost();
			}
		});
	}

	[[nodiscard]] IModuleNode* PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept override;

	[[nodiscard]] DropResult EvalDrop(const IModuleNode& node, DirectX::XMFLOAT2 worldPos, ZoneId from) const noexcept override;

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
	[[nodiscard]] static float HalfSpanX_() noexcept;
	[[nodiscard]] static float HalfSpanY_() noexcept;
	void EnsurePanelVisual_(Graphics& gfx, Rgph::RenderGraph& rg);
	void PaintPanel_();
	void SyncPanelTransform_() noexcept;

	std::vector<std::unique_ptr<IModuleNode>> nodes_;
	DirectX::XMFLOAT3 warehouseOrigin_{ 0.0f, 0.0f, 0.0f };
	std::unique_ptr<Canvas2D> panel_;
};
