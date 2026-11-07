#pragma once

#include "IModuleZone.h"

#include <cmath>
#include <cstddef>
#include <memory>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

// 商店里的一块（买卖 / 炼成 / 奥义）。不是区。
// 外壳怎么切、跨块的 Node 归谁，仍由 ModuleShop 决定。
// 这里放一块的矩形、画面，以及这一块自己持有的 Node。
class ModuleShopBlock
{
public:
	virtual ~ModuleShopBlock() = default;

	ModuleShopBlock(const ModuleShopBlock&) = delete;
	ModuleShopBlock& operator=(const ModuleShopBlock&) = delete;

	// 这一块的世界矩形。商店算好后交进来。
	void SetBounds(IModuleZone::BoundsWorld bounds) noexcept
	{
		bounds_ = bounds;
	}

	// 商店原点。块用它把世界坐标换成 Node 的 localPos。
	void SetHostOrigin(DirectX::XMFLOAT3 origin) noexcept
	{
		hostOrigin_ = origin;
	}

	// 点是否落在这一块的世界矩形里。判定和 ModuleShopDraw::PointInAabb_ 相同。
	[[nodiscard]] bool HitArea(DirectX::XMFLOAT2 worldPos) const noexcept
	{
		return std::fabs(worldPos.x - bounds_.center.x) <= bounds_.half.x
			&& std::fabs(worldPos.y - bounds_.center.y) <= bounds_.half.y;
	}

	// 记下画面设备，再让这一块按当前矩形建画布。
	void Ensure(Graphics& gfx, Rgph::RenderGraph& rg)
	{
		gfx_ = &gfx;
		rg_ = &rg;
		EnsureVisuals_();
	}

	// 需要时重画，并把底板和 Node 对齐到当前矩形。
	virtual void Sync() = 0;

	// 每帧入口。默认只对齐画面。有倒计时的块以后再覆盖。
	virtual void Tick(float)
	{
		Sync();
	}

	// 底板，画在 Node 下面。
	virtual void Submit() const = 0;

	// 画在 Node 上面的东西，例如钱数和锁按钮。没有就空着。
	virtual void SubmitOverlay() const {}

	// 换语言后重画文字。没有文字就空着。
	virtual void RefreshCopy() {}

	// 这颗 Node 的实体是否在这一块手里。别人停进来的不算。
	[[nodiscard]] virtual bool Owns(const IModuleNode* node) const noexcept = 0;

	// 从这一块取出并交出所有权。不是这块的就返回空。
	[[nodiscard]] virtual std::unique_ptr<IModuleNode> TakeNode(IModuleNode* node) = 0;

	// 点到这一块里可选的 Node 时返回它，并把距离平方写到 outDistSq。
	[[nodiscard]] virtual IModuleNode* PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept = 0;

	// 这一块交给商店 GetNode 的个数。奥义没有本体时是 0。
	[[nodiscard]] virtual std::size_t GetZoneNodeCount() const noexcept = 0;

	// 这一块内部的第 index 颗 Node。下标从 0 另计，不是商店的总下标。
	[[nodiscard]] virtual IModuleNode* GetZoneNode(std::size_t index) const noexcept = 0;

	// 提交这一块要画出来的 Node。
	virtual void SubmitNodes() = 0;

	// 把这一块的 Node 对齐到 hostOrigin_ 并同步画面。
	virtual void SyncNodes() = 0;

protected:
	ModuleShopBlock() = default;

	// Ensure 已经把 gfx_ 和 rg_ 记下。块在这里按 bounds_ 建自己的画布。
	virtual void EnsureVisuals_() = 0;

	// 这一块的世界矩形。
	IModuleZone::BoundsWorld bounds_{};
	// 商店原点。
	DirectX::XMFLOAT3 hostOrigin_{ 0.0f, 0.0f, 0.0f };
	// Ensure 之后才有效。画面还没建好时是空的。
	Graphics* gfx_{ nullptr };
	Rgph::RenderGraph* rg_{ nullptr };
};
