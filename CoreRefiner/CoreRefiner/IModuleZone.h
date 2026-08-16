#pragma once

#include "IFieldNode.h"

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

	[[nodiscard]] virtual ZoneId GetZoneId() const noexcept = 0;
	[[nodiscard]] virtual DirectX::XMFLOAT3 GetOrigin() const noexcept = 0;
	virtual void SetOrigin(DirectX::XMFLOAT3 origin) noexcept = 0;

	[[nodiscard]] virtual std::size_t GetNodeCount() const noexcept = 0;
	[[nodiscard]] virtual IFieldNode* GetNode(std::size_t index) const noexcept = 0;

	[[nodiscard]] virtual std::unique_ptr<IFieldNode> TakeNode(IFieldNode* node) = 0;

	[[nodiscard]] virtual bool TryAcceptDrop(
		std::unique_ptr<IFieldNode>& node,
		DirectX::XMFLOAT2 localPos) = 0;

	[[nodiscard]] virtual bool ContainsCircle(
		DirectX::XMFLOAT2 worldCenter,
		float radius) const noexcept = 0;

	[[nodiscard]] virtual IFieldNode* PickAt(
		DirectX::XMFLOAT2 worldPos,
		float& outDistSq) noexcept = 0;

	[[nodiscard]] virtual DropResult EvalDrop(
		const IFieldNode& node,
		DirectX::XMFLOAT2 worldPos,
		ZoneId from) const noexcept = 0;

	virtual void InitAllVisuals(
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		DirectX::XMFLOAT3 origin) = 0;
	virtual void SyncAllVisuals() = 0;
	virtual void SubmitBackground() = 0;
	virtual void SubmitNodes() = 0;
	virtual void ClearLayoutGhosts() = 0;

protected:
	IModuleZone() = default;
};
