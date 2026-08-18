#pragma once

#include "IModuleNode.h"

#include <cstddef>
#include <DirectXMath.h>
#include <memory>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

enum class ZoneId : unsigned char
{
	Field,
	Warehouse,
	Shop,
	Count
};

[[nodiscard]] inline constexpr std::size_t ZoneCount() noexcept
{
	return static_cast<std::size_t>(ZoneId::Count);
}

[[nodiscard]] inline constexpr std::size_t ToIndex(ZoneId id) noexcept
{
	return static_cast<std::size_t>(id);
}

enum class DropVerdict : unsigned char
{
	Accept,
	OutOfBounds,
	Blocked,
	NoSpace,
	Forbidden,
	Unaffordable,
};

struct DropResult
{
	DropVerdict verdict{ DropVerdict::OutOfBounds };
	DirectX::XMFLOAT2 localPos{ 0.0f, 0.0f };
};

[[nodiscard]] inline constexpr bool IsDropAccepted(DropVerdict verdict) noexcept
{
	return verdict == DropVerdict::Accept;
}

class IModuleZone
{
public:
	virtual ~IModuleZone() = default;

	IModuleZone(const IModuleZone&) = delete;
	IModuleZone& operator=(const IModuleZone&) = delete;

	/** @brief 世界空间轴对齐包围盒（中心 + 半宽半高）。 */
	struct BoundsWorld
	{
		DirectX::XMFLOAT2 center{ 0.0f, 0.0f };
		DirectX::XMFLOAT2 half{ 0.0f, 0.0f };
	};

	[[nodiscard]] virtual ZoneId GetZoneId() const noexcept = 0;
	[[nodiscard]] virtual DirectX::XMFLOAT3 GetOrigin() const noexcept = 0;
	virtual void SetOrigin(DirectX::XMFLOAT3 origin) noexcept = 0;

	/** @brief 内容框世界包围盒（命中与布局用）。 */
	[[nodiscard]] virtual BoundsWorld GetBoundsWorld() const noexcept = 0;

	/** @brief 外壳世界包围盒（仅绘制）；默认 = 内容框四周外扩 kShellPad。 */
	[[nodiscard]] virtual BoundsWorld GetShellBoundsWorld() const noexcept;

	[[nodiscard]] virtual std::size_t GetNodeCount() const noexcept = 0;
	[[nodiscard]] virtual IModuleNode* GetNode(std::size_t index) const noexcept = 0;

	[[nodiscard]] virtual std::size_t FindNodeIndex(const IModuleNode* node) const noexcept
	{
		if (node == nullptr)
		{
			return static_cast<std::size_t>(-1);
		}
		for (std::size_t i = 0; i < GetNodeCount(); ++i)
		{
			if (GetNode(i) == node)
			{
				return i;
			}
		}
		return static_cast<std::size_t>(-1);
	}

	[[nodiscard]] virtual std::unique_ptr<IModuleNode> TakeNode(IModuleNode* node) = 0;

	[[nodiscard]] virtual bool TryAcceptDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos) = 0;

	[[nodiscard]] virtual bool ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept = 0;

	[[nodiscard]] virtual IModuleNode* PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept = 0;

	[[nodiscard]] virtual DropResult EvalDrop(const IModuleNode& node, DirectX::XMFLOAT2 worldPos, ZoneId from) const noexcept = 0;

	/** @brief 初始化本区视觉：先定原点并确保外壳，再交给子类。 */
	void InitAllVisuals(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 origin);
	/** @brief 同步本区变换：先同步外壳，再交给子类。 */
	void SyncAllVisuals();
	/** @brief 提交本区背景：先提交外壳，再交给子类。 */
	void SubmitBackground();
	virtual void SubmitNodes() = 0;

	template <typename Fn>
	void ForEach(Fn&& fn)
	{
		for (std::size_t i = 0; i < GetNodeCount(); ++i)
		{
			if (IModuleNode* node = GetNode(i))
			{
				fn(*node);
			}
		}
	}

	template <typename Fn>
	void ForEach(Fn&& fn) const
	{
		for (std::size_t i = 0; i < GetNodeCount(); ++i)
		{
			if (IModuleNode* node = GetNode(i))
			{
				fn(*node);
			}
		}
	}

	virtual void ClearLayoutGhosts()
	{
		ForEach([](IModuleNode& node)
		{
			if (node.IsLayoutGhostActive())
			{
				node.EndLayoutGhost();
			}
		});
	}

protected:
	IModuleZone() = default;

	/** @brief 外壳相对内容框四周各扩的边距。 */
	static constexpr float kShellPad = 12.0f;

	/** @brief 子类初始化自身内容视觉（不含外壳）。 */
	virtual void InitZoneVisuals_(Graphics& gfx, Rgph::RenderGraph& rg) = 0;
	/** @brief 子类同步自身内容变换（不含外壳）。 */
	virtual void SyncZoneTransforms_() = 0;
	/** @brief 子类提交自身内容背景（不含外壳）。 */
	virtual void SubmitZoneBackground_() = 0;

	static void PutPixelClamped(Canvas2D& canvas, int x, int y, Color c);
	static void DrawHLine(Canvas2D& canvas, int x0, int x1, int y, Color c);
	static void DrawVLine(Canvas2D& canvas, int x, int y0, int y1, Color c);
	static void DrawRectOutline(Canvas2D& canvas, int x0, int y0, int x1, int y1, Color c);

private:
	void EnsureShell_(Graphics& gfx, Rgph::RenderGraph& rg);
	void PaintShell_();
	void SyncShellTransform_() noexcept;
	void SubmitShell_();

	std::unique_ptr<Canvas2D> shell_;
};
