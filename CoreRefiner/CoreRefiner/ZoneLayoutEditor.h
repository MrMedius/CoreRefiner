#pragma once

#include "Canvas2D.h"
#include "IModuleZone.h"
#include "NodeInfoPanel.h"
#include "Colors.h"

#include "Win.h"

#include <array>
#include <cstdint>
#include <DirectXMath.h>
#include <functional>
#include <memory>

// 布局编辑器请上层打开的叠加界面。卖出确认和更名会发；重绘以后也走这一条。
enum class OverlayUiKind : unsigned char
{
	SellConfirm,
	Rename,
	Redraw,
};

struct OverlayUiRequest
{
	OverlayUiKind kind{ OverlayUiKind::SellConfirm };
	std::uint32_t instanceId{ 0 };
	int sellPrice{ 0 };
};

class Window;
class Graphics;

namespace Rgph
{
	class RenderGraph;
}

class ZoneLayoutEditor
{
public:
	ZoneLayoutEditor() = default;
	~ZoneLayoutEditor() = default;

	ZoneLayoutEditor(const ZoneLayoutEditor&) = delete;
	ZoneLayoutEditor& operator=(const ZoneLayoutEditor&) = delete;

	void Begin(std::array<IModuleZone*, ZoneCount()> zones, std::array<DirectX::XMFLOAT3, ZoneCount()> origins, Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 combatFieldOrigin);

	void End();

	void SetOnOverlayRequest(std::function<void(const OverlayUiRequest&)> cb);

	// 按 instanceId 再找到要卖的 Node，然后卖掉。找不到就不卖。
	bool ConfirmSell(std::uint32_t instanceId);

	void Update(float dt, Window* hostWindow);

	// 关掉拖拽、悬停和信息面板。叠加界面打开时用。
	void ClearPointerState();

	void SubmitOverlay();

	[[nodiscard]] bool IsActive() const noexcept { return active_; }

private:
	static constexpr ZoneId kNoZone_{ ZoneId::Count };

	struct DropEval_
	{
		bool placeable{ false };
		bool unaffordable{ false };
		ZoneId target{ kNoZone_ };
	};

	void EnsureRingVisual_(Graphics& gfx, Rgph::RenderGraph& rg);
	[[nodiscard]] static std::unique_ptr<Canvas2D> MakeRingCanvas_(Graphics& gfx, Rgph::RenderGraph& rg, Color ringColor);

	void ClearAllLayoutGhosts_();
	[[nodiscard]] DirectX::XMFLOAT2 MouseGame_() const noexcept;
	[[nodiscard]] DirectX::XMFLOAT3 OriginForSource_(ZoneId source) const noexcept;
	[[nodiscard]] DirectX::XMFLOAT2 WorldPosOf_(const IModuleNode& node, DirectX::XMFLOAT3 origin) const noexcept;
	[[nodiscard]] IModuleZone* ZoneAt_(ZoneId id) const noexcept;
	[[nodiscard]] IModuleZone* OwnerZoneOf_(const IModuleNode* node) const noexcept;

	void SetFreePreview_(IModuleNode& node, DirectX::XMFLOAT2 mouseGame);

	[[nodiscard]] DropEval_ EvalDrop_(const IModuleNode& node) const noexcept;

	void RevertDrag_();
	void ResolveRelease_();
	void ClearActiveDrag_();
	// 卖价超过阈值：放回原位，只把编号和卖价交出去，不留 Node 指针。没超过返回 false。
	bool RequestSellConfirm_(IModuleNode& node);
	// 更名：只交当前奥义的编号，不留 Node 指针。
	void RequestRename_();
	// 这次松手会在买卖框卖掉时，超过阈值就改成请求确认。返回 true 表示已经拦住。
	bool TryDeferTradeSell_(IModuleNode& node, IModuleZone* target, DirectX::XMFLOAT2 world);
	[[nodiscard]] IModuleNode* FindByInstanceId_(std::uint32_t instanceId) const noexcept;

	[[nodiscard]] IModuleNode* PickHover_(DirectX::XMFLOAT2 mouseGame, ZoneId& outSource) const noexcept;

	void SnapCursorToNode_(IModuleNode& node, DirectX::XMFLOAT3 origin, Window& hostWindow) const noexcept;
	void SyncRingTransform_(IModuleNode& node, DirectX::XMFLOAT3 origin);
	void SyncOneRingTransform_(Canvas2D& ring, IModuleNode& node, DirectX::XMFLOAT3 origin) const;
	[[nodiscard]] Canvas2D* ActiveRing_() const noexcept;

	bool active_{ false };
	std::function<void(const OverlayUiRequest&)> onOverlay_{};

	DirectX::XMFLOAT3 combatOrigin_{ 200.0f, 200.0f, 0.0f };
	std::array<IModuleZone*, ZoneCount()> zones_{};

	DirectX::XMFLOAT2 dragStartLocalPos_{ 0.0f, 0.0f };
	DirectX::XMFLOAT3 dragOrigin_{ 0.0f, 0.0f, 0.0f };

	IModuleNode* hover_{ nullptr };
	IModuleNode* dragged_{ nullptr };
	ZoneId hoverSource_{ kNoZone_ };
	ZoneId dragSource_{ kNoZone_ };

	enum class RingKind_
	{
		Hover,   ///< Idle hover — yellow
		Valid,   ///< Dragging, placeable — green
		Overlap, ///< Dragging, not placeable — red
		Denied,  ///< Dragging, Unaffordable — gray
	};
	RingKind_ ringKind_{ RingKind_::Hover };
	std::unique_ptr<Canvas2D> ringHover_;
	std::unique_ptr<Canvas2D> ringValid_;
	std::unique_ptr<Canvas2D> ringOverlap_;
	std::unique_ptr<Canvas2D> ringDenied_;
	NodeInfoPanel infoPanel_;

	static constexpr float kRingPadding_{ 12.0f };
};