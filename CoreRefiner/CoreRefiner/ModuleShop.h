#pragma once
#include "Canvas2D.h"
#include "IModuleNode.h"
#include "IModuleZone.h"
#include "NodeInfoPanel.h"

#include <array>
#include <cstddef>
#include <DirectXMath.h>
#include <memory>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

class ModuleShop : public IModuleZone
{
public:
	static constexpr int kColumns = 5;
	static constexpr int kRows = 1;
	static constexpr std::size_t kSlotCount = static_cast<std::size_t>(kColumns) * static_cast<std::size_t>(kRows);
	static constexpr float kSlotPitch = 140.0f;
	static constexpr float kBoundsPad = 36.0f;
	static constexpr float kInfoMaxWidthPx = 140.0f;
	static constexpr float kPriceFontSize = 14.0f;
	static constexpr float kPriceGapBelowIcon = 2.0f;

	struct BoundsWorld
	{
		DirectX::XMFLOAT2 center{ 0.0f, 0.0f };
		DirectX::XMFLOAT2 half{ 0.0f, 0.0f };
	};

	struct Slot
	{
		std::unique_ptr<IModuleNode> node;
		bool sold{ false };
		int price{ 0 };
	};

	ModuleShop() = default;
	~ModuleShop() override = default;

	ModuleShop(const ModuleShop&) = delete;
	ModuleShop& operator=(const ModuleShop&) = delete;

	[[nodiscard]] ZoneId GetZoneId() const noexcept override { return ZoneId::Shop; }

	[[nodiscard]] std::size_t GetNodeCount() const noexcept override { return kSlotCount; }

	[[nodiscard]] IModuleNode* GetNode(std::size_t index) const noexcept override
	{
		return (index < kSlotCount) ? slots_[index].node.get() : nullptr;
	}

	[[nodiscard]] const Slot* GetSlot(std::size_t index) const noexcept
	{
		return (index < kSlotCount) ? &slots_[index] : nullptr;
	}

	[[nodiscard]] DirectX::XMFLOAT3 GetOrigin() const noexcept override { return origin_; }

	[[nodiscard]] BoundsWorld GetBoundsWorld() const noexcept;

	[[nodiscard]] bool ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept override;

	void SetOrigin(DirectX::XMFLOAT3 origin) noexcept override;

	void FillStock();

	void MarkSold(std::size_t index);

	[[nodiscard]] bool TryAcceptDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos) override;

	[[nodiscard]] std::unique_ptr<IModuleNode> TakeNode(IModuleNode* node) override;

	void InitAllVisuals(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 origin) override;
	void SyncAllVisuals() override;
	void SubmitBackground() override;
	void SubmitNodes() override;
	void SubmitInfoPanels();
	void SubmitAllVisuals();

	[[nodiscard]] IModuleNode* PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept override;

	[[nodiscard]] DropResult EvalDrop(
		const IModuleNode& node,
		DirectX::XMFLOAT2 worldPos,
		ZoneId from) const noexcept override;

private:
	[[nodiscard]] static DirectX::XMFLOAT2 SlotLocalPos_(std::size_t index) noexcept;
	[[nodiscard]] static float HalfSpanX_() noexcept;
	[[nodiscard]] static float HalfSpanY_() noexcept;
	void RelayoutSlots_();
	void SyncInfoPanels_();
	void EnsurePriceVisuals_();
	void PaintPrice_(std::size_t index);
	void SyncPriceLabels_();
	void SubmitPrices_();
	void EnsurePanelVisual_(Graphics& gfx, Rgph::RenderGraph& rg);
	void PaintPanel_();
	void SyncPanelTransform_() noexcept;
	[[nodiscard]] std::size_t FindSlotIndex_(const IModuleNode* node) const noexcept;

	std::array<Slot, kSlotCount> slots_{};
	std::array<NodeInfoPanel, kSlotCount> infoPanels_{};
	std::array<std::unique_ptr<Canvas2D>, kSlotCount> priceCanvases_{};
	std::array<int, kSlotCount> paintedPrice_{};
	DirectX::XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
	Graphics* gfx_{ nullptr };
	Rgph::RenderGraph* rg_{ nullptr };
	std::unique_ptr<Canvas2D> panel_;
};
