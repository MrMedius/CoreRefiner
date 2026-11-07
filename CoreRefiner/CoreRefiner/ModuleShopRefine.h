#pragma once

#include "Canvas2D.h"
#include "IModuleNode.h"
#include "IModuleZone.h"
#include "ModuleShopBlock.h"

#include <array>
#include <cstddef>
#include <memory>

// 商店炼成区：三格停放、结果与两颗店有 Node、四个操作和底板。
// 商店通过 refine_ 调用这里。格子按 bounds_ 和 hostOrigin_ 计算。
// 货架商品由商店拒绝后再问 CanPark。退回后的奥义摆位由商店刷新奥义区完成。
class ModuleShopRefine : public ModuleShopBlock
{
public:
	// ---- 前置 ----
	// 停放格个数。0 素材，1 主体，2 结果。没命中时返回这个值。
	static constexpr std::size_t kParkCount = 3;
	// 右侧按钮个数。0 升级，1 合成，2 进化，3 退回。
	static constexpr std::size_t kButtonCount = 4;
	// 交给商店 GetNode 的个数。0 是结果，1 和 2 是停在素材、主体上的店有 Node。空位也占下标。
	// 这和停放格的 0 素材、1 主体、2 结果不是同一套下标。
	static constexpr std::size_t kZoneNodeCount = 3;

	ModuleShopRefine() = default;
	~ModuleShopRefine() override = default;

	ModuleShopRefine(const ModuleShopRefine&) = delete;
	ModuleShopRefine& operator=(const ModuleShopRefine&) = delete;

	// 宿主区。找主人、取出 Node、结果格的归属都问它。商店传入自己。
	void SetHostZone(IModuleZone* host) noexcept;

	// ---- 重载 ----
	// 按钮亮灭或持有的钱变了就重画，再把底板对齐到 bounds_。基类 Tick 会调到这里。
	void Sync() override;
	// 提交炼成底板。画在 Node 下面。
	void Submit() const override;
	// 换语言后重画格子标题和四个按钮。
	void RefreshCopy() override;

	// 结果或两颗店有 Node 的 unique_ptr 在这块手里。停进来的场、仓、奥义不算。
	[[nodiscard]] bool Owns(const IModuleNode* node) const noexcept override;
	// 取出结果或店有的素材、主体。不是这块持有的返回空。
	[[nodiscard]] std::unique_ptr<IModuleNode> TakeNode(IModuleNode* node) override;
	// 点在某个停放格里时返回那颗 Node，并把距离平方写到 outDistSq。场和仓停进来的也能点到。
	[[nodiscard]] IModuleNode* PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept override;
	// 固定交出 kZoneNodeCount 个下标，空位也算。
	[[nodiscard]] std::size_t GetZoneNodeCount() const noexcept override;
	// 下标 0 是结果，1 和 2 是店有的素材、主体。空位返回空。
	[[nodiscard]] IModuleNode* GetZoneNode(std::size_t index) const noexcept override;
	// 提交结果和两颗店有 Node。停进来的场、仓、奥义由它们自己的区提交。
	void SubmitNodes() override;
	// 把结果和两颗店有 Node 对齐到 hostOrigin_ 并同步画面。
	void SyncNodes() override;

	// ---- 独有 · 停放 ----
	// 点落在哪一格。没命中返回 kParkCount。
	[[nodiscard]] std::size_t HitParkSlot(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 第 slot 格中心的世界坐标。越界时返回这块的中心。
	[[nodiscard]] DirectX::XMFLOAT2 SlotWorldCenter(std::size_t slot) const noexcept;
	// 这颗 Node 能不能停进这一格。货架拒绝不在这里，由商店先判断。
	[[nodiscard]] bool CanPark(const IModuleNode& node, std::size_t slot) const noexcept;
	// 把 Node 停进这一格。结果格只接店有 Node。owner 用来把世界中心写成 localPos。
	void Park(std::size_t slot, IModuleNode& node, IModuleZone* owner);
	// 从停放格摘掉这颗 Node，清掉格内图标大小，并重画。
	void Unbind(IModuleNode* node) noexcept;
	// 素材和主体退回原处。结果格留下，直到清空结果或被拖走。
	void ClearParks();
	// 丢掉结果和两颗店有 Node，并重画。
	void ClearResult();
	// 拖出失败时把 Node 放回结果格。
	void RestoreResult(std::unique_ptr<IModuleNode> node);
	// 拖出失败时放回这一格。0 和 1 进店有素材、主体，其余进结果。
	void RestoreHeld(std::unique_ptr<IModuleNode> node, std::size_t slot);
	// 这颗 Node 在第几格。结果和店有素材、主体也算。不在时返回 kParkCount。
	[[nodiscard]] std::size_t FindParkIndex(const IModuleNode* node) const noexcept;
	// 三格停放指针里有没有这颗 Node。
	[[nodiscard]] bool IsParked(const IModuleNode* node) const noexcept;
	// 这颗 Node 是不是结果格里的那一颗。
	[[nodiscard]] bool IsResultParked(const IModuleNode* node) const noexcept;
	// 第 slot 格当前停着的 Node。越界或空位返回空。
	[[nodiscard]] IModuleNode* GetParked(std::size_t slot) const noexcept;
	// 只清掉指向这颗 Node 的停放指针。不重画。卖掉奥义或结束本局时用。
	void ForgetParked(const IModuleNode* node) noexcept;
	// 按当前格子的边长设置格内图标半径。
	void ApplyParkIcon(IModuleNode& node) noexcept;
	// 店有 Node 的残影钉在结果格，图标用格内尺寸。跟手图标由调用方清掉。
	void PinHomeGhost(IModuleNode& node) noexcept;
	// 把结果格里的店有 Node 按当前栏位重摆。外壳移动或改尺寸时调用。
	void RealignResult();

	// ---- 独有 · 按钮 ----
	// 点是否落在右侧四个按钮之一上。
	[[nodiscard]] bool HitButton(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 点到升级、合成、进化或退回就执行。field 和 warehouse 用来把素材、主体交还原主人。
	bool TryClick(DirectX::XMFLOAT2 worldPos, IModuleZone* field, IModuleZone* warehouse);

private:
	// ---- 重载 ----
	// 按 bounds_ 建炼成底板并画一次。gfx_ 和 rg_ 已由基类记下。
	void EnsureVisuals_() override;

	// ---- 独有 · 布局 ----
	// 这一帧炼成区的像素布局。三格和右侧四键都从这里量。
	struct Layout
	{
		IModuleZone::BoundsWorld bounds{};
		int cw{ 1 };
		int ch{ 1 };
		int pad{ 0 };
		int side{ 0 };
		int slotTop{ 0 };
		int slotXs[kParkCount]{};
		int btnW{ 120 };
		int btnH{ 14 };
		int btnX{ 0 };
		int btnTop{ 0 };
		int btnGap{ 5 };
	};

	// 按当前 bounds_ 和底板像素尺寸做出一份 Layout。
	[[nodiscard]] Layout CurrentLayout_() const noexcept;
	// 在画布像素里排三格和右侧四键。
	static void FillLayout_(Layout& layout) noexcept;
	// 世界坐标换成画布像素。宽或高无效时返回 false。
	[[nodiscard]] bool WorldToPixel_(DirectX::XMFLOAT2 world, const Layout& layout, float& px, float& py) const noexcept;
	// 第 slot 格中心的世界坐标。越界时返回这块的中心。
	[[nodiscard]] DirectX::XMFLOAT2 SlotCenterFrom_(const Layout& layout, std::size_t slot) const noexcept;
	// 按格子边长换算图标世界半径。和仓库格同一比例。
	[[nodiscard]] float SlotRadiusFrom_(const Layout& layout) const noexcept;
	// 点落在哪一格。没命中返回 kParkCount。
	[[nodiscard]] std::size_t HitSlotIndex_(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 点落在哪一颗按钮。没命中返回 kButtonCount。
	[[nodiscard]] std::size_t HitButtonIndex_(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 当前布局下的图标世界半径。
	[[nodiscard]] float SlotRadius_() const noexcept;

	// ---- 独有 · 绘制 ----
	// 空格子标题的字号。
	static constexpr float kSlotLabelFont_{ 20.0f };
	// 四键亮灭或持有的钱和上次画的不一样。
	[[nodiscard]] bool NeedsPaint_() const noexcept;
	// 画底板、三格、箭头和四个按钮，并记下这次的亮灭和金额。
	void Paint_();
	// 把底板的位置和缩放对齐到 bounds_。
	void SyncTransform_() noexcept;

	// ---- 独有 · 停放 ----
	// 把 Node 摆进这一格：localPos、原点，以及格内图标大小。结果格同时结束残影。
	void PlacePark_(std::size_t slot, IModuleNode& node, IModuleZone* owner);
	// 店有的回到结果格。其余清掉停放并结束残影。
	void BouncePark_(IModuleNode& node);
	// 店有 Node 从结果或其他进料格挪到这一格，残影钉在结果格。
	void RelocateOwnedToFeed_(std::size_t slot, IModuleNode& node);
	// 店有 Node 收回结果格。不是店有的返回 false。
	bool ReturnOwnedToResult_(IModuleNode& node);
	// 收下操作产出的 Node，放进结果格并重画。
	void AdoptResult_(std::unique_ptr<IModuleNode> node);
	// 结果格有 Node 时，按宿主区把它摆进结果格。
	void PlaceResultVisual_();

	// ---- 独有 · 操作 ----
	// 素材和主体都在，结果格空着，并且钱够操作费。
	[[nodiscard]] bool HasPairReady_() const noexcept;
	// 素材等级不低于主体，主体还没满级。
	[[nodiscard]] bool CanUpgrade_() const noexcept;
	// 两边都满级、Label 不同，并且都不是融合或核心。
	[[nodiscard]] bool CanFuse_() const noexcept;
	// 两边都满级，并且对得上进化表。
	[[nodiscard]] bool CanEvolve_() const noexcept;
	// 素材或主体至少有一格有 Node。
	[[nodiscard]] bool CanReturn_() const noexcept;
	// 这颗 Node 现在归宿主、场、仓中的哪一个。
	[[nodiscard]] IModuleZone* FindOwner_(IModuleNode* node, IModuleZone* field, IModuleZone* warehouse) const noexcept;

	// 一次操作从两格取出的 Node，以及它们原来的主人。
	struct TakenPair_
	{
		std::unique_ptr<IModuleNode> material;
		std::unique_ptr<IModuleNode> subject;
		IModuleZone* materialOwner{ nullptr };
		IModuleZone* subjectOwner{ nullptr };
	};

	// 扣操作费，并从主人那里取出素材和主体。
	bool TryTakePair_(IModuleZone* field, IModuleZone* warehouse, TakenPair_& out);
	// 主体升级，造价改为两颗买入价之和，放进结果格。
	bool TryUpgrade_(IModuleZone* field, IModuleZone* warehouse);
	// 两颗合成一颗融合，放进结果格。
	bool TryFuse_(IModuleZone* field, IModuleZone* warehouse);
	// 按进化表造一颗 1 级新 Node，造价为两颗买入价之和，放进结果格。
	bool TryEvolve_(IModuleZone* field, IModuleZone* warehouse);
	// 把素材和主体退回原处。
	bool TryReturn_();

	// ---- 数据 · 宿主 ----
	// 商店自己。找主人和结果格归属用。还没设置时为空。
	IModuleZone* hostZone_{ nullptr };

	// ---- 数据 · 底板 ----
	// 炼成底板，画在 Node 下面。
	std::unique_ptr<Canvas2D> panel_;

	// ---- 数据 · 停放 ----
	// 三格当前指着的 Node。可以是场、仓、奥义或店有 Node。
	std::array<IModuleNode*, kParkCount> parked_{};
	// 结果格的实体。没有结果时为空。
	std::unique_ptr<IModuleNode> result_;
	// 停在素材、主体的店有 Node。和结果互斥，至多一颗有实体。
	std::array<std::unique_ptr<IModuleNode>, kZoneNodeCount - 1> feedOwned_{};

	// ---- 数据 · 按钮 ----
	// 上次画上去的四键亮灭。和当前判断不一致时重画。
	bool paintedUpgradeLit_{ false };
	bool paintedFuseLit_{ false };
	bool paintedEvolveLit_{ false };
	bool paintedReturnLit_{ false };
	// 上次画费用时持有的钱。-1 表示还没画。
	int paintedCurrency_{ -1 };
};
