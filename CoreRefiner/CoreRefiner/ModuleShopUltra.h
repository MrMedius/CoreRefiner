#pragma once

#include "Canvas2D.h"
#include "IModuleNode.h"
#include "IModuleZone.h"
#include "ModuleShopBlock.h"

#include <cstddef>
#include <cstdint>
#include <memory>

class ModuleNode_Ultra;

// 商店第三块的奥义区。矩形、原点和画布生命周期走 ModuleShopBlock。
// 锻造、打孔、更名、重绘和左右翻页仍在这里处理。圆点不能点。
class ModuleShopUltra : public ModuleShopBlock
{
public:
	enum class Button : unsigned char
	{
		None,
		Forge,
		Punch,
		Rename,
		Redraw,
		PageLeft,
		PageRight,
	};

	ModuleShopUltra() = default;
	~ModuleShopUltra() override = default;

	ModuleShopUltra(const ModuleShopUltra&) = delete;
	ModuleShopUltra& operator=(const ModuleShopUltra&) = delete;

	// 需要时重画，并把奥义底板和看得见的内部 Node 对齐。
	void Sync() override;
	// 提交奥义底板。画在 Node 下面。
	void Submit() const override;

	void Adopt(std::unique_ptr<IModuleNode> node);
	[[nodiscard]] std::unique_ptr<IModuleNode> Release();
	// 结束本局：丢掉奥义和里面的 Node，滚动回到第一格。
	void ClearForRunEnd();

	[[nodiscard]] IModuleNode* GetUltra() noexcept;
	[[nodiscard]] const IModuleNode* GetUltra() const noexcept;
	[[nodiscard]] bool HasUltra() const noexcept;

	[[nodiscard]] DirectX::XMFLOAT2 GetUltraSlotCenter() const noexcept;
	[[nodiscard]] float GetUltraSlotRadius() const noexcept;
	[[nodiscard]] DirectX::XMFLOAT2 GetInnerSlotCenter(std::size_t index) const noexcept;
	[[nodiscard]] float GetInnerSlotRadius() const noexcept;

	[[nodiscard]] int GetInnerSlotCount() const noexcept;
	[[nodiscard]] IModuleNode* GetInnerSlot(std::size_t index) noexcept;
	[[nodiscard]] const IModuleNode* GetInnerSlot(std::size_t index) const noexcept;
	// 从 scroll_ 起，当前能画在右侧按钮左边的内部栏位。
	[[nodiscard]] bool IsInnerSlotVisible(std::size_t index) const noexcept;

	// 鼠标落在当前画出来的按钮上。没有奥义时只有锻造；有奥义时是右侧三个和左右三角。圆点不算。
	[[nodiscard]] Button HitButton(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 点到锻造、打孔、更名、重绘，或还能翻的三角就执行。到头的三角不响应。位置用 hostOrigin_。
	bool TryClick(DirectX::XMFLOAT2 worldPos);
	// 这次点击是更名时返回 true，并清掉记号。编辑器据此发请求，不看按钮种类。
	bool TakeRenameRequest() noexcept;
	// 这次点击是重绘时返回 true，并清掉记号。编辑器据此发请求，不看按钮种类。
	bool TakeRedrawRequest() noexcept;
	// 钱够且还没有奥义时扣 3 元，造一颗默认奥义放进左侧栏位。位置按 hostOrigin_ 换算。
	bool TryForge();
	// 已有奥义且钱够时扣 1 元，容量加 1，并重画栏位。
	bool TryPunch();
	// 点在奥义栏位圆里时返回奥义本体。内部栏位以后再加。
	[[nodiscard]] const IModuleNode* PickInfoNode(DirectX::XMFLOAT2 worldPos) const noexcept;

	// 奥义本体，或任意内部栏里的 Node（含滚出画面的）。
	[[nodiscard]] bool Owns(const IModuleNode* node) const noexcept override;
	// 残影钉在奥义栏或内部栏。坐标按 hostOrigin_ 换算。跟手图标由调用方清掉 Icon 覆盖。
	void PinHomeGhost(IModuleNode& node) noexcept;
	// 取出失败时放回奥义栏，并按栏位大小显示。位置按 hostOrigin_ 换算。
	void Restore(std::unique_ptr<IModuleNode> node);
	// 内部栏下标。不在里面时返回 npos。
	[[nodiscard]] std::size_t FindInnerIndex(const IModuleNode* node) const noexcept;
	// 取出失败时放回这个内部栏，并按小一圈显示。位置按 hostOrigin_ 换算。
	void RestoreInner(std::unique_ptr<IModuleNode> node, std::size_t index);
	// 空的奥义栏接奥义。看得见的空内部栏接能放入的 Node。正在拖的那一格也算空。其余拒绝。落点按 hostOrigin_ 换算。
	[[nodiscard]] DropResult EvalDrop(const IModuleNode& node, DirectX::XMFLOAT2 worldPos) const noexcept;

	enum class DropClaim : unsigned char
	{
		Ignore,
		Reject,
		Accept,
	};

	// localPos 落在栏位中心才处理。Ignore 表示不是奥义区的落点。摆放用 hostOrigin_。
	[[nodiscard]] DropClaim TryAcceptDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos);
	[[nodiscard]] std::unique_ptr<IModuleNode> TakeNode(IModuleNode* node) override;
	// 奥义本体和当前看得见的内部 Node。
	[[nodiscard]] IModuleNode* PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept override;
	// 没有奥义时是 0。有奥义时先算本体，再算看得见且不为空的内部 Node。
	[[nodiscard]] std::size_t GetZoneNodeCount() const noexcept override;
	// 下标 0 是奥义本体。后面只排看得见且不为空的内部 Node。
	[[nodiscard]] IModuleNode* GetZoneNode(std::size_t index) const noexcept override;
	// 提交奥义本体和看得见的内部 Node。
	void SubmitNodes() override;
	// 把本体和看得见的内部 Node 对齐到 hostOrigin_。滚出画面的不在这里同步。
	void SyncNodes() override;

private:
	struct Layout
	{
		IModuleZone::BoundsWorld bounds{};
		int cw = 1;
		int ch = 1;
		int pad = 0;
		int ultraX = 0;
		int ultraY = 0;
		int ultraSide = 0;
		int innerSide = 0;
		int innerGap = 0;
		int innerX0 = 0;
		int innerY = 0;
		int capacity = 0;
		int windowCount = 0;
		int visibleBegin = 0;
		int visibleCount = 0;
		int btnX = 0;
		int btnY = 0;
		int btnW = 0;
		int btnH = 0;
		int btnGap = 0;
		int forgeX = 0;
		int forgeY = 0;
		int forgeW = 0;
		int forgeH = 0;
		// 内部栏下面、奥义栏和右侧按钮之间的翻页条。
		int pagerX = 0;
		int pagerY = 0;
		int pagerW = 0;
		int pagerH = 0;
	};

	// 按当前边界建画布。货币、容量、滚动或栏位内容变了会重画。gfx_ 和 rg_ 已由 ModuleShopBlock 记下。
	void EnsureVisuals_() override;
	[[nodiscard]] Layout BuildLayout_() const noexcept;
	[[nodiscard]] bool NeedsPaint_() const noexcept;
	void Paint_();
	static void DrawPager_(Canvas2D& canvas, const Layout& layout) noexcept;
	static void FillLayout_(Layout& layout) noexcept;
	[[nodiscard]] static int FitInnerSlots_(const Layout& layout) noexcept;
	[[nodiscard]] int ClampedScroll_(const Layout& layout) const noexcept;
	// 点是否落在奥义区整块里。矩形判断交给 ModuleShopBlock。
	[[nodiscard]] bool InBand_(DirectX::XMFLOAT2 worldPos) const noexcept;
	[[nodiscard]] bool HitUltraSlot_(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 看得见的内部栏，返回容量下标。没点中返回 -1。
	[[nodiscard]] int HitVisibleInnerSlot_(DirectX::XMFLOAT2 worldPos) const noexcept;
	// direction 为负看更靠前的格子，为正看更靠后的。到头返回 false。
	bool TryPage_(int direction);
	void RealignVisible_();
	[[nodiscard]] static DirectX::XMFLOAT2 PixelCenterToWorld_(const Layout& layout, float px, float py) noexcept;
	[[nodiscard]] static bool WorldToPixel_(const Layout& layout, DirectX::XMFLOAT2 world, float& px, float& py) noexcept;
	[[nodiscard]] static float SlotRadius_(const Layout& layout, int side) noexcept;

	std::unique_ptr<Canvas2D> panel_;
	std::unique_ptr<IModuleNode> ultra_;
	int scroll_{ 0 };
	// 更名按钮刚被点下。TakeRenameRequest 取走后清掉。
	bool renameRequested_{ false };
	// 重绘按钮刚被点下。TakeRedrawRequest 取走后清掉。
	bool redrawRequested_{ false };
	bool painted_ = false;
	int paintedScroll_ = 0;
	bool paintedHas_ = false;
	int paintedCurrency_ = 0;
	int paintedCapacity_ = -1;
	std::uint32_t paintedRevision_ = 0;
};
