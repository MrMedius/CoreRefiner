#include "ModuleShopUltra.h"

#include "Channels.h"
#include "GameRulesCodex.h"
#include "GameStatsCodex.h"
#include "ModuleNodeFactory.h"
#include "ModuleNodes.h"
#include "ModuleShopDraw.h"
#include "UiCopy.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace
{
	// ---- 本体 ----
	// 把 Node 认成奥义。不是奥义时为空。
	[[nodiscard]] const ModuleNode_Ultra* AsUltra_(const IModuleNode* node) noexcept
	{
		return dynamic_cast<const ModuleNode_Ultra*>(node);
	}

	[[nodiscard]] ModuleNode_Ultra* AsUltra_(IModuleNode* node) noexcept
	{
		return dynamic_cast<ModuleNode_Ultra*>(node);
	}

	// ---- 落点 ----
	// 世界坐标换成相对商店原点的 localPos。
	[[nodiscard]] DirectX::XMFLOAT2 ToLocal_(DirectX::XMFLOAT3 origin, DirectX::XMFLOAT2 world) noexcept
	{
		return DirectX::XMFLOAT2{ world.x - origin.x, world.y - origin.y };
	}

	// 两个 localPos 是否近到可以当成同一个落点。
	[[nodiscard]] bool NearLocal_(DirectX::XMFLOAT2 a, DirectX::XMFLOAT2 b) noexcept
	{
		const float dx = a.x - b.x;
		const float dy = a.y - b.y;
		return dx * dx + dy * dy <= 16.0f;
	}

	// 把 Node 摆进栏位：位置、原点，以及栏位大小的图标。
	void PlantInSlot_(IModuleNode& node, DirectX::XMFLOAT3 origin, DirectX::XMFLOAT2 local, float radius)
	{
		node.SetLocalPos(local);
		node.SetZoneOrigin(origin);
		node.SetZoneVisualScale(1.0f);
		node.SetVisualRadiusOverride(radius);
		node.SetIconRadiusOverride(radius);
		node.SyncVisual();
	}

	// 离开栏位时清掉栏位强加的图标大小。
	void ClearSlotVisual_(IModuleNode& node)
	{
		node.ClearIconRadiusOverride();
		node.ClearVisualRadiusOverride();
	}

	// ---- 点击 ----
	// 像素点是否落在这个矩形里。宽或高不大于 0 时不算。
	[[nodiscard]] bool InRect_(float px, float py, int x, int y, int w, int h) noexcept
	{
		return w > 0 && h > 0
			&& px >= static_cast<float>(x)
			&& py >= static_cast<float>(y)
			&& px < static_cast<float>(x + w)
			&& py < static_cast<float>(y + h);
	}

}


// ---- 公开 · 重载 ----

void ModuleShopUltra::Sync()
{
	if (NeedsPaint_())
	{
		Paint_();
	}
	ModuleShopDraw::SyncBandTransform_(panel_.get(), bounds_);
	RealignVisible_();
}

void ModuleShopUltra::Submit() const
{
	if (panel_ != nullptr)
	{
		panel_->Submit(Chan::ui);
	}
}

bool ModuleShopUltra::Owns(const IModuleNode* node) const noexcept
{
	if (node == nullptr)
	{
		return false;
	}
	if (ultra_.get() == node)
	{
		return true;
	}
	const int count = GetInnerSlotCount();
	for (int i = 0; i < count; ++i)
	{
		if (GetInnerSlot(static_cast<std::size_t>(i)) == node)
		{
			return true;
		}
	}
	return false;
}

std::unique_ptr<IModuleNode> ModuleShopUltra::TakeNode(IModuleNode* node)
{
	if (node == nullptr)
	{
		return nullptr;
	}
	if (ultra_.get() == node)
	{
		std::unique_ptr<IModuleNode> taken = Release();
		if (taken != nullptr)
		{
			ClearSlotVisual_(*taken);
		}
		return taken;
	}
	ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	if (ultra == nullptr)
	{
		return nullptr;
	}
	std::unique_ptr<IModuleNode> taken = ultra->Take(node);
	if (taken == nullptr)
	{
		return nullptr;
	}
	ClearSlotVisual_(*taken);
	if (panel_ != nullptr)
	{
		Paint_();
	}
	return taken;
}

IModuleNode* ModuleShopUltra::PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept
{
	IModuleNode* best = nullptr;
	float bestDistSq = 1.0e9f;

	// 点选范围是画出来的整格奥义栏，空奥义也能从这一格拖出来。
	if (IModuleNode* body = GetUltra(); body != nullptr && HitUltraSlot_(worldPos))
	{
		const DirectX::XMFLOAT2 center = GetUltraSlotCenter();
		const float dx = worldPos.x - center.x;
		const float dy = worldPos.y - center.y;
		const float distSq = dx * dx + dy * dy;
		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			best = body;
		}
	}
	// 内部栏点选整格，空出来的边也能把 Node 拖出来。
	if (const int inner = HitVisibleInnerSlot_(worldPos); inner >= 0)
	{
		const std::size_t index = static_cast<std::size_t>(inner);
		if (IModuleNode* held = GetInnerSlot(index))
		{
			const DirectX::XMFLOAT2 center = GetInnerSlotCenter(index);
			const float dx = worldPos.x - center.x;
			const float dy = worldPos.y - center.y;
			const float distSq = dx * dx + dy * dy;
			if (distSq < bestDistSq)
			{
				bestDistSq = distSq;
				best = held;
			}
		}
	}
	if (best != nullptr)
	{
		outDistSq = bestDistSq;
	}
	return best;
}

std::size_t ModuleShopUltra::GetZoneNodeCount() const noexcept
{
	if (!HasUltra())
	{
		return 0;
	}
	std::size_t count = 1;
	const int innerCount = GetInnerSlotCount();
	for (int i = 0; i < innerCount; ++i)
	{
		const std::size_t slot = static_cast<std::size_t>(i);
		if (IsInnerSlotVisible(slot) && GetInnerSlot(slot) != nullptr)
		{
			++count;
		}
	}
	return count;
}

IModuleNode* ModuleShopUltra::GetZoneNode(std::size_t index) const noexcept
{
	if (!HasUltra())
	{
		return nullptr;
	}
	// const 方法要交出可改的 Node，和商店的 GetNode 一样。
	if (index == 0)
	{
		return const_cast<IModuleNode*>(GetUltra());
	}
	std::size_t cursor = 1;
	const int innerCount = GetInnerSlotCount();
	for (int i = 0; i < innerCount; ++i)
	{
		const std::size_t slot = static_cast<std::size_t>(i);
		if (!IsInnerSlotVisible(slot))
		{
			continue;
		}
		IModuleNode* node = const_cast<IModuleNode*>(GetInnerSlot(slot));
		if (node == nullptr)
		{
			continue;
		}
		if (index == cursor)
		{
			return node;
		}
		++cursor;
	}
	return nullptr;
}

void ModuleShopUltra::SubmitNodes()
{
	if (IModuleNode* body = GetUltra())
	{
		body->SubmitVisual();
	}
	const int innerCount = GetInnerSlotCount();
	for (int i = 0; i < innerCount; ++i)
	{
		const std::size_t slot = static_cast<std::size_t>(i);
		if (!IsInnerSlotVisible(slot))
		{
			continue;
		}
		if (IModuleNode* node = GetInnerSlot(slot))
		{
			node->SubmitVisual();
		}
	}
}

void ModuleShopUltra::SyncNodes()
{
	auto syncOwned = [this](IModuleNode* node)
	{
		if (node == nullptr)
		{
			return;
		}
		node->SetZoneOrigin(hostOrigin_);
		node->SyncVisual();
	};
	syncOwned(GetUltra());
	const int innerCount = GetInnerSlotCount();
	for (int i = 0; i < innerCount; ++i)
	{
		const std::size_t slot = static_cast<std::size_t>(i);
		if (!IsInnerSlotVisible(slot))
		{
			continue;
		}
		syncOwned(GetInnerSlot(slot));
	}
}

// ---- 公开 · 独有 · 本体 ----

void ModuleShopUltra::Adopt(std::unique_ptr<IModuleNode> node)
{
	ultra_ = std::move(node);
	painted_ = false;
	if (panel_ != nullptr)
	{
		Paint_();
	}
	RealignVisible_();
}

std::unique_ptr<IModuleNode> ModuleShopUltra::Release()
{
	painted_ = false;
	std::unique_ptr<IModuleNode> out = std::move(ultra_);
	if (panel_ != nullptr)
	{
		Paint_();
	}
	return out;
}

void ModuleShopUltra::ClearForRunEnd()
{
	scroll_ = 0;
	std::unique_ptr<IModuleNode> dropped = Release();
	dropped.reset();
}

IModuleNode* ModuleShopUltra::GetUltra() noexcept
{
	return ultra_.get();
}

const IModuleNode* ModuleShopUltra::GetUltra() const noexcept
{
	return ultra_.get();
}

bool ModuleShopUltra::HasUltra() const noexcept
{
	return ultra_ != nullptr;
}

// ---- 公开 · 独有 · 栏位 ----

DirectX::XMFLOAT2 ModuleShopUltra::GetUltraSlotCenter() const noexcept
{
	const Layout layout = BuildLayout_();
	const float px = static_cast<float>(layout.ultraX) + static_cast<float>(layout.ultraSide) * 0.5f;
	const float py = static_cast<float>(layout.ultraY) + static_cast<float>(layout.ultraSide) * 0.5f;
	return PixelCenterToWorld_(layout, px, py);
}

float ModuleShopUltra::GetUltraSlotRadius() const noexcept
{
	const Layout layout = BuildLayout_();
	return SlotRadius_(layout, layout.ultraSide);
}

DirectX::XMFLOAT2 ModuleShopUltra::GetInnerSlotCenter(std::size_t index) const noexcept
{
	const Layout layout = BuildLayout_();
	if (layout.innerSide <= 0 || index >= static_cast<std::size_t>(layout.capacity))
	{
		return layout.bounds.center;
	}
	const int stride = layout.innerSide + layout.innerGap;
	const int delta = static_cast<int>(index) - layout.visibleBegin;
	const float px = static_cast<float>(layout.innerX0 + delta * stride) + static_cast<float>(layout.innerSide) * 0.5f;
	const float py = static_cast<float>(layout.innerY) + static_cast<float>(layout.innerSide) * 0.5f;
	return PixelCenterToWorld_(layout, px, py);
}

float ModuleShopUltra::GetInnerSlotRadius() const noexcept
{
	const Layout layout = BuildLayout_();
	return SlotRadius_(layout, layout.innerSide);
}

int ModuleShopUltra::GetInnerSlotCount() const noexcept
{
	const ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	return (ultra != nullptr) ? ultra->GetCapacity() : 0;
}

IModuleNode* ModuleShopUltra::GetInnerSlot(std::size_t index) noexcept
{
	ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	return (ultra != nullptr) ? ultra->GetSlot(index) : nullptr;
}

const IModuleNode* ModuleShopUltra::GetInnerSlot(std::size_t index) const noexcept
{
	const ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	return (ultra != nullptr) ? ultra->GetSlot(index) : nullptr;
}

bool ModuleShopUltra::IsInnerSlotVisible(std::size_t index) const noexcept
{
	const Layout layout = BuildLayout_();
	if (index >= static_cast<std::size_t>(layout.capacity))
	{
		return false;
	}
	const int slot = static_cast<int>(index);
	return slot >= layout.visibleBegin && slot < layout.visibleBegin + layout.visibleCount;
}

// ---- 公开 · 独有 · 按钮 ----

ModuleShopUltra::Button ModuleShopUltra::HitButton(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const Layout layout = BuildLayout_();
	float px = 0.0f;
	float py = 0.0f;
	if (!WorldToPixel_(layout, worldPos, px, py))
	{
		return Button::None;
	}
	if (ultra_ == nullptr)
	{
		return InRect_(px, py, layout.forgeX, layout.forgeY, layout.forgeW, layout.forgeH)
			? Button::Forge
			: Button::None;
	}
	const Button buttons[ModuleShopDraw::kUltraButtonCount] = { Button::Punch, Button::Rename, Button::Redraw };
	for (int i = 0; i < ModuleShopDraw::kUltraButtonCount; ++i)
	{
		const int y = layout.btnY + i * (layout.btnH + layout.btnGap);
		if (InRect_(px, py, layout.btnX, y, layout.btnW, layout.btnH))
		{
			return buttons[i];
		}
	}
	const ModuleShopDraw::UltraPagerGlyphs glyphs = ModuleShopDraw::BuildUltraPagerGlyphs(
		layout.pagerX,
		layout.pagerY,
		layout.pagerW,
		layout.pagerH,
		layout.capacity);
	if (!glyphs.valid)
	{
		return Button::None;
	}
	if (InRect_(px, py, glyphs.leftX, glyphs.triY, glyphs.triW, glyphs.triH))
	{
		return Button::PageLeft;
	}
	if (InRect_(px, py, glyphs.rightX, glyphs.triY, glyphs.triW, glyphs.triH))
	{
		return Button::PageRight;
	}
	return Button::None;
}

bool ModuleShopUltra::TryClick(DirectX::XMFLOAT2 worldPos)
{
	switch (HitButton(worldPos))
	{
	case Button::Forge:
		return TryForge();
	case Button::Punch:
		return TryPunch();
	case Button::Rename:
		if (ultra_ == nullptr)
		{
			return false;
		}
		// 只记一笔。编辑器取走后再请工作台打开更名界面。
		renameRequested_ = true;
		return true;
	case Button::Redraw:
		if (ultra_ == nullptr)
		{
			return false;
		}
		// 只记一笔。编辑器取走后再请工作台打开重绘界面。
		redrawRequested_ = true;
		return true;
	case Button::PageLeft:
		return TryPage_(-1);
	case Button::PageRight:
		return TryPage_(1);
	default:
		return false;
	}
}

bool ModuleShopUltra::TakeRenameRequest() noexcept
{
	const bool requested = renameRequested_;
	renameRequested_ = false;
	return requested;
}

bool ModuleShopUltra::TakeRedrawRequest() noexcept
{
	const bool requested = redrawRequested_;
	redrawRequested_ = false;
	return requested;
}

bool ModuleShopUltra::TryForge()
{
	if (ultra_ != nullptr || gfx_ == nullptr || rg_ == nullptr)
	{
		return false;
	}
	if (GameStatsCodex::GetCurrency() < GameRulesCodex::ForgeCost)
	{
		return false;
	}

	const DirectX::XMFLOAT2 center = GetUltraSlotCenter();
	const float radius = GetUltraSlotRadius();
	std::unique_ptr<IModuleNode> node = ModuleNodeFactory::MakeUltra(DirectX::XMFLOAT2{
		center.x - hostOrigin_.x,
		center.y - hostOrigin_.y
	});
	if (node == nullptr)
	{
		return false;
	}
	node->InitVisual(*gfx_, *rg_, hostOrigin_);
	node->SetZoneVisualScale(1.0f);
	node->SetVisualRadiusOverride(radius);
	node->SetIconRadiusOverride(radius);
	node->SyncVisual();
	if (!GameStatsCodex::TrySpendCurrency(GameRulesCodex::ForgeCost))
	{
		return false;
	}
	Adopt(std::move(node));
	return true;
}

bool ModuleShopUltra::TryPunch()
{
	ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	if (ultra == nullptr || GameStatsCodex::GetCurrency() < GameRulesCodex::PunchCost)
	{
		return false;
	}
	if (!GameStatsCodex::TrySpendCurrency(GameRulesCodex::PunchCost))
	{
		return false;
	}
	ultra->AddCapacity();
	if (panel_ != nullptr)
	{
		Paint_();
	}
	RealignVisible_();
	return true;
}

const IModuleNode* ModuleShopUltra::PickInfoNode(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const IModuleNode* ultra = GetUltra();
	if (ultra == nullptr)
	{
		return nullptr;
	}
	const DirectX::XMFLOAT2 center = GetUltraSlotCenter();
	const float radius = GetUltraSlotRadius();
	const float dx = worldPos.x - center.x;
	const float dy = worldPos.y - center.y;
	if (dx * dx + dy * dy > radius * radius)
	{
		return nullptr;
	}
	return ultra;
}

// ---- 公开 · 独有 · 拖放 ----

void ModuleShopUltra::PinHomeGhost(IModuleNode& node) noexcept
{
	DirectX::XMFLOAT2 center = GetUltraSlotCenter();
	float radius = GetUltraSlotRadius();
	if (ultra_.get() != &node)
	{
		const std::size_t index = FindInnerIndex(&node);
		if (index == static_cast<std::size_t>(-1))
		{
			return;
		}
		center = GetInnerSlotCenter(index);
		radius = GetInnerSlotRadius();
	}
	const DirectX::XMFLOAT2 local = ToLocal_(hostOrigin_, center);
	node.SetZoneOrigin(hostOrigin_);
	node.SetLocalPos(local);
	node.BeginLayoutGhost(local);
	node.SetVisualRadiusOverride(radius);
	node.SyncVisual();
}

void ModuleShopUltra::Restore(std::unique_ptr<IModuleNode> node)
{
	if (node == nullptr || node->GetKind() != ModuleNodeKind::Ultra || HasUltra())
	{
		return;
	}
	node->EndLayoutGhost();
	PlantInSlot_(*node, hostOrigin_, ToLocal_(hostOrigin_, GetUltraSlotCenter()), GetUltraSlotRadius());
	Adopt(std::move(node));
}

std::size_t ModuleShopUltra::FindInnerIndex(const IModuleNode* node) const noexcept
{
	if (node == nullptr)
	{
		return static_cast<std::size_t>(-1);
	}
	const int count = GetInnerSlotCount();
	for (int i = 0; i < count; ++i)
	{
		if (GetInnerSlot(static_cast<std::size_t>(i)) == node)
		{
			return static_cast<std::size_t>(i);
		}
	}
	return static_cast<std::size_t>(-1);
}

void ModuleShopUltra::RestoreInner(std::unique_ptr<IModuleNode> node, std::size_t index)
{
	if (node == nullptr)
	{
		return;
	}
	ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	node->EndLayoutGhost();
	if (ultra == nullptr || !ultra->TryPut(index, node))
	{
		return;
	}
	if (IModuleNode* placed = GetInnerSlot(index))
	{
		PlantInSlot_(*placed, hostOrigin_, ToLocal_(hostOrigin_, GetInnerSlotCenter(index)), GetInnerSlotRadius());
	}
	if (panel_ != nullptr)
	{
		Paint_();
	}
}

DropResult ModuleShopUltra::EvalDrop(const IModuleNode& node, DirectX::XMFLOAT2 worldPos) const noexcept
{
	DropResult result{};
	result.verdict = DropVerdict::Forbidden;
	// 正在拖这颗奥义时，它还占着栏位，但整块奥义区仍应能把它放回去。
	if (!HasUltra() || ultra_.get() == &node)
	{
		// 没奥义时整块奥义区都能接，松手后仍落在左侧栏位中心。
		if (node.GetKind() == ModuleNodeKind::Ultra && InBand_(worldPos))
		{
			result.verdict = DropVerdict::Accept;
			result.localPos = ToLocal_(hostOrigin_, GetUltraSlotCenter());
			return result;
		}
		if (!HasUltra())
		{
			return result;
		}
	}
	if (HitUltraSlot_(worldPos))
	{
		// 已有奥义时，只有画出来的奥义栏讨论另一颗奥义，并且拒绝。
		return result;
	}

	const int inner = HitVisibleInnerSlot_(worldPos);
	if (inner < 0)
	{
		return result;
	}
	const ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	const IModuleNode* held = GetInnerSlot(static_cast<std::size_t>(inner));
	// 正在从这一格拖走的 Node 不算占用，可以放回原格。
	if (ultra == nullptr || !ultra->CanHold(node) || (held != nullptr && held != &node))
	{
		return result;
	}
	result.verdict = DropVerdict::Accept;
	const DirectX::XMFLOAT2 center = GetInnerSlotCenter(static_cast<std::size_t>(inner));
	result.localPos = ToLocal_(hostOrigin_, center);
	return result;
}

ModuleShopUltra::DropClaim ModuleShopUltra::TryAcceptDrop(
	std::unique_ptr<IModuleNode>& node,
	DirectX::XMFLOAT2 localPos)
{
	if (node == nullptr)
	{
		return DropClaim::Ignore;
	}

	const DirectX::XMFLOAT2 ultraLocal = ToLocal_(hostOrigin_, GetUltraSlotCenter());
	if (NearLocal_(localPos, ultraLocal))
	{
		if (HasUltra() || node->GetKind() != ModuleNodeKind::Ultra)
		{
			return DropClaim::Reject;
		}
		PlantInSlot_(*node, hostOrigin_, ultraLocal, GetUltraSlotRadius());
		Adopt(std::move(node));
		return DropClaim::Accept;
	}

	const int count = GetInnerSlotCount();
	for (int i = 0; i < count; ++i)
	{
		const std::size_t index = static_cast<std::size_t>(i);
		if (!IsInnerSlotVisible(index))
		{
			continue;
		}
		const DirectX::XMFLOAT2 slotLocal = ToLocal_(hostOrigin_, GetInnerSlotCenter(index));
		if (!NearLocal_(localPos, slotLocal))
		{
			continue;
		}
		ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
		if (ultra == nullptr || !ultra->TryPut(index, node))
		{
			return DropClaim::Reject;
		}
		if (IModuleNode* placed = GetInnerSlot(index))
		{
			PlantInSlot_(*placed, hostOrigin_, slotLocal, GetInnerSlotRadius());
		}
		if (panel_ != nullptr)
		{
			Paint_();
		}
		return DropClaim::Accept;
	}
	return DropClaim::Ignore;
}

// ---- 私有 · 重载 ----

void ModuleShopUltra::EnsureVisuals_()
{
	const unsigned w = static_cast<unsigned>(std::lround((std::max)(1.0f, bounds_.half.x * 2.0f)));
	const unsigned h = static_cast<unsigned>(std::lround((std::max)(1.0f, bounds_.half.y * 2.0f)));
	const bool resized = ModuleShopDraw::EnsureCanvasSize_(panel_, *gfx_, *rg_, w, h);
	if (resized || NeedsPaint_())
	{
		Paint_();
	}
	Sync();
}

// ---- 私有 · 独有 · 布局 ----

ModuleShopUltra::Layout ModuleShopUltra::BuildLayout_() const noexcept
{
	Layout layout{};
	layout.bounds = bounds_;
	layout.cw = (std::max)(1, static_cast<int>(std::lround(bounds_.half.x * 2.0f)));
	layout.ch = (std::max)(1, static_cast<int>(std::lround(bounds_.half.y * 2.0f)));
	if (panel_ != nullptr)
	{
		const int pw = static_cast<int>(panel_->GetCanvasWidth());
		const int ph = static_cast<int>(panel_->GetCanvasHeight());
		if (pw > 0)
		{
			layout.cw = pw;
		}
		if (ph > 0)
		{
			layout.ch = ph;
		}
	}
	const ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	layout.capacity = (ultra != nullptr) ? ultra->GetCapacity() : 0;
	// 像素位置只看画布宽高和容量。当前页在下面按 scroll_ 截断。
	ModuleShopDraw::FillUltraLayout(layout.cw, layout.ch, layout.capacity, layout);
	const int begin = ClampedScroll_(layout);
	layout.visibleBegin = begin;
	const int remain = layout.capacity - begin;
	layout.visibleCount = (remain > 0) ? (std::min)(layout.windowCount, remain) : 0;
	return layout;
}

int ModuleShopUltra::ClampedScroll_(const Layout& layout) const noexcept
{
	const int maxBegin = (std::max)(0, layout.capacity - layout.windowCount);
	int begin = scroll_;
	if (begin < 0)
	{
		begin = 0;
	}
	if (begin > maxBegin)
	{
		begin = maxBegin;
	}
	return begin;
}

DirectX::XMFLOAT2 ModuleShopUltra::PixelCenterToWorld_(const Layout& layout, float px, float py) noexcept
{
	const float worldW = layout.bounds.half.x * 2.0f;
	const float worldH = layout.bounds.half.y * 2.0f;
	if (layout.cw <= 0 || layout.ch <= 0 || worldW <= 1.0e-4f || worldH <= 1.0e-4f)
	{
		return layout.bounds.center;
	}
	return DirectX::XMFLOAT2{
		layout.bounds.center.x - layout.bounds.half.x + px * (worldW / static_cast<float>(layout.cw)),
		layout.bounds.center.y - layout.bounds.half.y + py * (worldH / static_cast<float>(layout.ch))
	};
}

bool ModuleShopUltra::WorldToPixel_(const Layout& layout, DirectX::XMFLOAT2 world, float& px, float& py) noexcept
{
	const float worldW = layout.bounds.half.x * 2.0f;
	const float worldH = layout.bounds.half.y * 2.0f;
	if (worldW <= 1.0e-4f || worldH <= 1.0e-4f || layout.cw <= 0 || layout.ch <= 0)
	{
		return false;
	}
	px = (world.x - (layout.bounds.center.x - layout.bounds.half.x)) * (static_cast<float>(layout.cw) / worldW);
	py = (world.y - (layout.bounds.center.y - layout.bounds.half.y)) * (static_cast<float>(layout.ch) / worldH);
	return true;
}

float ModuleShopUltra::SlotRadius_(const Layout& layout, int side) noexcept
{
	if (side <= 0 || layout.ch <= 0)
	{
		return ModuleShopDraw::kUltraIconRadiusMin;
	}
	// worldSide 是这一栏边长在世界上的长度。
	const float worldSide = static_cast<float>(side) * ((layout.bounds.half.y * 2.0f) / static_cast<float>(layout.ch));
	return ModuleShopDraw::UltraSlotIconRadius(worldSide);
}

// ---- 私有 · 独有 · 绘制 ----

bool ModuleShopUltra::NeedsPaint_() const noexcept
{
	if (panel_ == nullptr || !painted_)
	{
		return true;
	}
	const ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	const bool has = ultra_ != nullptr;
	const int capacity = (ultra != nullptr) ? ultra->GetCapacity() : 0;
	const std::uint32_t revision = (ultra != nullptr) ? ultra->GetContentRevision() : 0u;
	return has != paintedHas_
		|| capacity != paintedCapacity_
		|| revision != paintedRevision_
		|| scroll_ != paintedScroll_
		|| GameStatsCodex::GetCurrency() != paintedCurrency_;
}

void ModuleShopUltra::Paint_()
{
	if (panel_ == nullptr)
	{
		return;
	}

	Canvas2D& canvas = *panel_;
	ModuleShopDraw::PaintBandChrome_(canvas);
	const Layout layout = BuildLayout_();
	const int currency = GameStatsCodex::GetCurrency();
	const ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());

	if (ultra_ == nullptr)
	{
		// 无奥义：区域正中只有锻造。钱够才亮，亮了数字变黄。
		const bool lit = currency >= GameRulesCodex::ForgeCost;
		ModuleShopDraw::DrawActionButton_(
			canvas,
			layout.forgeX,
			layout.forgeY,
			layout.forgeW,
			layout.forgeH,
			GetUiCopy("shop.ultra.forge"),
			GameRulesCodex::ForgeCost,
			lit);
	}
	else
	{
		ModuleShopDraw::DrawSlotSquare_(canvas, layout.ultraX, layout.ultraY, layout.ultraSide);
		const int stride = layout.innerSide + layout.innerGap;
		for (int i = 0; i < layout.visibleCount; ++i)
		{
			const int x = layout.innerX0 + i * stride;
			ModuleShopDraw::DrawSlotSquare_(canvas, x, layout.innerY, layout.innerSide);
		}
		ModuleShopDraw::DrawUltraPager(canvas, layout, layout.visibleBegin, layout.visibleCount);

		const std::string captions[ModuleShopDraw::kUltraButtonCount] = {
			GetUiCopy("shop.ultra.punch"),
			GetUiCopy("shop.ultra.rename"),
			GetUiCopy("shop.ultra.redraw")
		};
		const int costs[ModuleShopDraw::kUltraButtonCount] = { GameRulesCodex::PunchCost, -1, -1 };
		const bool lits[ModuleShopDraw::kUltraButtonCount] = {
			currency >= GameRulesCodex::PunchCost,
			true,
			true
		};
		for (int i = 0; i < ModuleShopDraw::kUltraButtonCount; ++i)
		{
			const int y = layout.btnY + i * (layout.btnH + layout.btnGap);
			ModuleShopDraw::DrawActionButton_(canvas, layout.btnX, y, layout.btnW, layout.btnH, captions[i], costs[i], lits[i]);
		}
	}

	painted_ = true;
	paintedHas_ = ultra_ != nullptr;
	paintedScroll_ = scroll_;
	paintedCurrency_ = currency;
	paintedCapacity_ = (ultra != nullptr) ? ultra->GetCapacity() : 0;
	paintedRevision_ = (ultra != nullptr) ? ultra->GetContentRevision() : 0u;
	canvas.NotifyPixelsChanged();
}

// ---- 私有 · 独有 · 点击和翻页 ----

bool ModuleShopUltra::InBand_(DirectX::XMFLOAT2 worldPos) const noexcept
{
	return HitArea(worldPos);
}

bool ModuleShopUltra::HitUltraSlot_(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const Layout layout = BuildLayout_();
	float px = 0.0f;
	float py = 0.0f;
	if (!WorldToPixel_(layout, worldPos, px, py))
	{
		return false;
	}
	return InRect_(px, py, layout.ultraX, layout.ultraY, layout.ultraSide, layout.ultraSide);
}

int ModuleShopUltra::HitVisibleInnerSlot_(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const Layout layout = BuildLayout_();
	if (ultra_ == nullptr || layout.visibleCount <= 0)
	{
		return -1;
	}
	float px = 0.0f;
	float py = 0.0f;
	if (!WorldToPixel_(layout, worldPos, px, py))
	{
		return -1;
	}
	const int stride = layout.innerSide + layout.innerGap;
	for (int i = 0; i < layout.visibleCount; ++i)
	{
		const int x = layout.innerX0 + i * stride;
		if (InRect_(px, py, x, layout.innerY, layout.innerSide, layout.innerSide))
		{
			return layout.visibleBegin + i;
		}
	}
	return -1;
}

bool ModuleShopUltra::TryPage_(int direction)
{
	if (direction == 0 || ultra_ == nullptr)
	{
		return false;
	}
	const Layout layout = BuildLayout_();
	const int current = ClampedScroll_(layout);
	const int maxBegin = (std::max)(0, layout.capacity - layout.windowCount);
	int next = current + ((direction < 0) ? -1 : 1);
	if (next < 0)
	{
		next = 0;
	}
	if (next > maxBegin)
	{
		next = maxBegin;
	}
	if (next == current)
	{
		return false;
	}
	scroll_ = next;
	if (panel_ != nullptr)
	{
		Paint_();
	}
	RealignVisible_();
	return true;
}

// ---- 私有 · 独有 · 对齐 ----

void ModuleShopUltra::RealignVisible_()
{
	const int count = GetInnerSlotCount();
	if (count <= 0)
	{
		return;
	}
	const float radius = GetInnerSlotRadius();
	for (int i = 0; i < count; ++i)
	{
		const std::size_t index = static_cast<std::size_t>(i);
		IModuleNode* node = GetInnerSlot(index);
		if (node == nullptr || node->IsLayoutGhostActive() || !IsInnerSlotVisible(index))
		{
			continue;
		}
		if (!node->IsVisualReady() && gfx_ != nullptr && rg_ != nullptr)
		{
			node->InitVisual(*gfx_, *rg_, hostOrigin_);
		}
		PlantInSlot_(*node, hostOrigin_, ToLocal_(hostOrigin_, GetInnerSlotCenter(index)), radius);
	}
}
