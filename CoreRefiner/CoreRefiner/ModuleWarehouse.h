#pragma once
#include "Canvas2D.h"
#include "IModuleNode.h"
#include "IModuleZone.h"

#include <array>
#include <cstddef>
#include <DirectXMath.h>
#include <memory>
#include <utility>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

class ModuleWarehouse : public IModuleZone
{
public:
	/** @brief 5 列与 Field 等宽（half.x = 150 → 框宽 300）。 */
	static constexpr int kColumns = 5;
	static constexpr int kMaxRows = 4;
	static constexpr std::size_t kMaxSlots = static_cast<std::size_t>(kColumns) * static_cast<std::size_t>(kMaxRows);
	/** @brief 槽距 56 / pad 38：half = (150, 122)，框 300 x 244，四边留 10px。 */
	static constexpr float kSlotPitch = 56.0f;
	/** @brief 单格命中半宽；与 kSlotPitch/2 相等时格子无缝平铺。 */
	static constexpr float kCellHalfExtent = 28.0f;
	static constexpr float kBoundsPad = 38.0f;
	/** @brief 开局启用格数；容量只增不减，由 SetEnabledSlotCount 覆盖。 */
	static constexpr std::size_t kInitialEnabledSlots = 5;
	/** @brief 仓库内统一绘制半径；拿起后恢复节点真实命中半径。 */
	static constexpr float kStoredVisualRadius = 15.0f;

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
		const std::size_t slot = FindFirstEmptySlot_();
		if (slot >= kMaxSlots)
		{
			return nullptr;
		}
		T* raw = node.get();
		nodes_[slot] = std::move(node);
		RelayoutSlots();
		return raw;
	}

	[[nodiscard]] ZoneId GetZoneId() const noexcept override { return ZoneId::Warehouse; }

	/** @brief 物理槽位数；空槽返回 nullptr，供 ForEach / FindNodeIndex 遍历。 */
	[[nodiscard]] std::size_t GetNodeCount() const noexcept override { return kMaxSlots; }

	/** @brief 实际占用件数。 */
	[[nodiscard]] std::size_t GetOccupiedCount() const noexcept
	{
		std::size_t n = 0;
		for (const auto& slot : nodes_)
		{
			if (slot != nullptr)
			{
				++n;
			}
		}
		return n;
	}

	[[nodiscard]] bool IsFull() const noexcept { return GetOccupiedCount() >= enabledSlots_; }

	[[nodiscard]] bool HasFreeSlot() const noexcept { return !IsFull(); }

	[[nodiscard]] IModuleNode* GetNode(std::size_t index) const noexcept override
	{
		return (index < kMaxSlots) ? nodes_[index].get() : nullptr;
	}

	[[nodiscard]] std::size_t GetEnabledSlotCount() const noexcept { return enabledSlots_; }
	/** @brief 设置启用格数，取值钳制到 [0, kMaxSlots]。 */
	void SetEnabledSlotCount(std::size_t count) noexcept;

	[[nodiscard]] DirectX::XMFLOAT3 GetOrigin() const noexcept override { return warehouseOrigin_; }

	[[nodiscard]] BoundsWorld GetBoundsWorld() const noexcept override;

	[[nodiscard]] bool ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept override;

	void SetOrigin(DirectX::XMFLOAT3 origin) noexcept override;

	void RelayoutSlots();

	[[nodiscard]] bool TryAcceptDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos) override;

	[[nodiscard]] std::unique_ptr<IModuleNode> TakeNode(std::size_t index);
	[[nodiscard]] std::unique_ptr<IModuleNode> TakeNode(IModuleNode* node) override;

	void SubmitNodes() override;

	[[nodiscard]] IModuleNode* PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept override;

	[[nodiscard]] DropResult EvalDrop(const IModuleNode& node, DirectX::XMFLOAT2 worldPos, ZoneId from) const noexcept override;
	void OnSameZoneMove(IModuleNode& node, DirectX::XMFLOAT2 localPos) override;

protected:
	void InitZoneVisuals_(Graphics& gfx, Rgph::RenderGraph& rg) override;
	void SyncZoneTransforms_() override;
	void SubmitZoneBackground_() override;

private:
	[[nodiscard]] static DirectX::XMFLOAT2 SlotLocalPos_(std::size_t index) noexcept;
	[[nodiscard]] static float HalfSpanX_() noexcept;
	[[nodiscard]] static float HalfSpanY_() noexcept;
	/** @brief 第一个启用且为空的槽；满则返回 kMaxSlots。 */
	[[nodiscard]] std::size_t FindFirstEmptySlot_() const noexcept;
	/** @brief 世界坐标所在槽；未命中格子返回 kMaxSlots。 */
	[[nodiscard]] std::size_t SlotIndexAt_(DirectX::XMFLOAT2 worldPos) const noexcept;
	/** @brief 仓库本地坐标所在槽；未命中格子返回 kMaxSlots。 */
	[[nodiscard]] std::size_t SlotIndexAtLocal_(DirectX::XMFLOAT2 localPos) const noexcept;
	void EnsurePanelVisual_(Graphics& gfx, Rgph::RenderGraph& rg);
	void PaintPanel_();
	void SyncPanelTransform_() noexcept;

	std::array<std::unique_ptr<IModuleNode>, kMaxSlots> nodes_{};
	std::size_t enabledSlots_{ kInitialEnabledSlots };
	DirectX::XMFLOAT3 warehouseOrigin_{ 0.0f, 0.0f, 0.0f };
	std::unique_ptr<Canvas2D> panel_;
};