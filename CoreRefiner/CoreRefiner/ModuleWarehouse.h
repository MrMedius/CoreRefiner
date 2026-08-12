#pragma once
#include "Canvas2D.h"
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

/**
 * @brief Pause-only Node stockpile (non-Core). Owns IFieldNode instances in a capped grid.
 */
class ModuleWarehouse
{
public:
	static constexpr int kColumns = 3;
	static constexpr int kMaxRows = 4;
	static constexpr std::size_t kMaxSlots =
		static_cast<std::size_t>(kColumns) * static_cast<std::size_t>(kMaxRows);
	static constexpr float kSlotPitch = 48.0f;
	/** @brief Extra padding beyond outermost slot centers so a hit circle fits inside the box. */
	static constexpr float kBoundsPad = 20.0f;

	/**
	 * @brief Axis-aligned warehouse drop zone in game pixels (center + half extents).
	 */
	struct BoundsWorld
	{
		DirectX::XMFLOAT2 center{ 0.0f, 0.0f };
		DirectX::XMFLOAT2 half{ 0.0f, 0.0f };
	};

	ModuleWarehouse() = default;
	~ModuleWarehouse() = default;

	ModuleWarehouse(const ModuleWarehouse&) = delete;
	ModuleWarehouse& operator=(const ModuleWarehouse&) = delete;

	/**
	 * @brief Construct and store a node, then relayout.
	 * @return Raw pointer, or nullptr if Core / full (node discarded on Core; not created on full).
	 */
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

	[[nodiscard]] std::size_t GetNodeCount() const noexcept { return nodes_.size(); }

	[[nodiscard]] bool IsFull() const noexcept { return nodes_.size() >= kMaxSlots; }

	[[nodiscard]] bool HasFreeSlot() const noexcept { return !IsFull(); }

	[[nodiscard]] IFieldNode* GetNode(std::size_t index) const noexcept
	{
		return (index < nodes_.size()) ? nodes_[index].get() : nullptr;
	}

	[[nodiscard]] DirectX::XMFLOAT3 GetOrigin() const noexcept { return warehouseOrigin_; }

	/**
	 * @brief World-space AABB covering a full kMaxSlots grid (for drop containment).
	 */
	[[nodiscard]] BoundsWorld GetBoundsWorld() const noexcept;

	/**
	 * @brief True if circle (center, radius) lies entirely inside GetBoundsWorld().
	 */
	[[nodiscard]] bool ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept;

	void SetOrigin(DirectX::XMFLOAT3 origin) noexcept;

	void RelayoutSlots();

	/**
	 * @brief Adopt @p node if non-null, not Core, and not full; on failure leaves @p node unchanged.
	 */
	[[nodiscard]] IFieldNode* TryAdopt(std::unique_ptr<IFieldNode>& node);

	[[nodiscard]] std::unique_ptr<IFieldNode> TakeNode(std::size_t index);
	[[nodiscard]] std::unique_ptr<IFieldNode> TakeNode(IFieldNode* node);

	void InitAllVisuals(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 origin);
	void SyncAllVisuals();
	void SubmitBackground();
	void SubmitNodes();
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
	[[nodiscard]] static float HalfSpanX_() noexcept;
	[[nodiscard]] static float HalfSpanY_() noexcept;
	void EnsurePanelVisual_(Graphics& gfx, Rgph::RenderGraph& rg);
	void PaintPanel_();
	void SyncPanelTransform_() noexcept;

	std::vector<std::unique_ptr<IFieldNode>> nodes_;
	DirectX::XMFLOAT3 warehouseOrigin_{ 0.0f, 0.0f, 0.0f };
	std::unique_ptr<Canvas2D> panel_;
};
