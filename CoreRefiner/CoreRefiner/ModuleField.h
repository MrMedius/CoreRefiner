#pragma once

#include "IModuleNode.h"
#include "IModuleZone.h"
#include "ModuleFieldCanvas.h"

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

class ModuleField : public IModuleZone
{
public:
	static constexpr float kHalfExtent = 150.0f;

	ModuleField() = default;
	~ModuleField() override = default;

	ModuleField(const ModuleField&) = delete;
	ModuleField& operator=(const ModuleField&) = delete;

	template <typename T, typename... Args>
	T* AddNode(Args&&... args)
	{
		auto node = std::make_unique<T>(std::forward<Args>(args)...);
		T* raw = node.get();
		nodes_.push_back(std::move(node));
		return raw;
	}

	[[nodiscard]] IModuleNode* GetCore() const noexcept
	{
		for (const auto& n : nodes_)
		{
			if (n != nullptr && n->IsCore())
			{
				return n.get();
			}
		}
		return nullptr;
	}

	[[nodiscard]] ZoneId GetZoneId() const noexcept override { return ZoneId::Field; }

	[[nodiscard]] std::size_t GetNodeCount() const noexcept override { return nodes_.size(); }

	[[nodiscard]] IModuleNode* GetNode(std::size_t index) const noexcept override
	{
		return (index < nodes_.size()) ? nodes_[index].get() : nullptr;
	}

	[[nodiscard]] DirectX::XMFLOAT3 GetOrigin() const noexcept override { return origin_; }

	/** @brief origin 即中心，半宽半高均为 kHalfExtent。 */
	[[nodiscard]] BoundsWorld GetBoundsWorld() const noexcept override;

	[[nodiscard]] ModuleFieldCanvas* GetCanvas() noexcept { return canvas_.get(); }
	[[nodiscard]] const ModuleFieldCanvas* GetCanvas() const noexcept { return canvas_.get(); }

	[[nodiscard]] std::unique_ptr<IModuleNode> TakeNode(std::size_t index)
	{
		if (index >= nodes_.size())
		{
			return nullptr;
		}
		std::unique_ptr<IModuleNode> out = std::move(nodes_[index]);
		nodes_.erase(nodes_.begin() + static_cast<std::ptrdiff_t>(index));
		return out;
	}

	[[nodiscard]] std::unique_ptr<IModuleNode> TakeNode(IModuleNode* node) override
	{
		if (node == nullptr)
		{
			return nullptr;
		}
		for (std::size_t i = 0; i < nodes_.size(); ++i)
		{
			if (nodes_[i].get() == node)
			{
				return TakeNode(i);
			}
		}
		return nullptr;
	}

	[[nodiscard]] bool TryAcceptDrop(
		std::unique_ptr<IModuleNode>& node,
		DirectX::XMFLOAT2 localPos) override;

	void TickAllCooldowns(float dt)
	{
		for (auto& n : nodes_)
		{
			if (n != nullptr)
			{
				n->TickCooldown(dt);
			}
		}
	}

	void ResetAllCooldowns() noexcept
	{
		for (auto& n : nodes_)
		{
			if (n != nullptr)
			{
				n->ResetCooldown();
			}
		}
	}

	void ClearWaves() noexcept
	{
		if (canvas_ != nullptr)
		{
			canvas_->ClearWaves();
		}
	}

	void SetWavesLocal(
		const DirectX::XMFLOAT2* centers,
		const float* radii,
		unsigned count,
		float fieldSide = ModuleFieldCanvas::kDefaultFieldSide) noexcept
	{
		if (canvas_ != nullptr)
		{
			canvas_->SetWavesLocal(centers, radii, count, fieldSide);
		}
	}

	void SetOrigin(DirectX::XMFLOAT3 origin) noexcept override;

	void SubmitNodes() override;

	void SubmitAllVisuals();

	[[nodiscard]] bool ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept override;

	[[nodiscard]] DirectX::XMFLOAT2 ClampLocalForRadius(DirectX::XMFLOAT2 localPos, float hitRadius) const noexcept;

	[[nodiscard]] bool WouldOverlap(const IModuleNode& self, DirectX::XMFLOAT2 fieldLocal) const noexcept;

	[[nodiscard]] IModuleNode* PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept override;

	[[nodiscard]] DropResult EvalDrop(const IModuleNode& node, DirectX::XMFLOAT2 worldPos, ZoneId from) const noexcept override;

protected:
	void InitZoneVisuals_(Graphics& gfx, Rgph::RenderGraph& rg) override;
	void SyncZoneTransforms_() override;
	void SubmitZoneBackground_() override;

private:
	std::vector<std::unique_ptr<IModuleNode>> nodes_;
	std::unique_ptr<ModuleFieldCanvas> canvas_;
	DirectX::XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
};