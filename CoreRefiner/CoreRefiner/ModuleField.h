#pragma once

#include "IFieldNode.h"
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

/**
 * @brief Combat loadout container: owns nodes + ModuleFieldCanvas background.
 */
class ModuleField
{
public:
	static constexpr float kHalfExtent = 150.0f;

	ModuleField() = default;
	~ModuleField() = default;

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

	[[nodiscard]] IFieldNode* GetCore() const noexcept
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

	[[nodiscard]] std::size_t GetNodeCount() const noexcept { return nodes_.size(); }

	[[nodiscard]] IFieldNode* GetNode(std::size_t index) const noexcept
	{
		return (index < nodes_.size()) ? nodes_[index].get() : nullptr;
	}

	[[nodiscard]] DirectX::XMFLOAT3 GetFieldOrigin() const noexcept { return origin_; }

	[[nodiscard]] ModuleFieldCanvas* GetCanvas() noexcept { return canvas_.get(); }
	[[nodiscard]] const ModuleFieldCanvas* GetCanvas() const noexcept { return canvas_.get(); }

	[[nodiscard]] std::unique_ptr<IFieldNode> TakeNode(std::size_t index)
	{
		if (index >= nodes_.size())
		{
			return nullptr;
		}
		std::unique_ptr<IFieldNode> out = std::move(nodes_[index]);
		nodes_.erase(nodes_.begin() + static_cast<std::ptrdiff_t>(index));
		return out;
	}

	[[nodiscard]] std::unique_ptr<IFieldNode> TakeNode(IFieldNode* node)
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

	IFieldNode* AdoptNode(std::unique_ptr<IFieldNode> node)
	{
		if (node == nullptr)
		{
			return nullptr;
		}
		IFieldNode* raw = node.get();
		nodes_.push_back(std::move(node));
		return raw;
	}

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

	/** @brief Ready all nodes and clear remaining cooldown (scene leave). */
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

	template <typename Fn>
	void ForEach(Fn&& fn)
	{
		for (auto& n : nodes_)
		{
			if (n != nullptr)
			{
				fn(*n);
			}
		}
	}

	template <typename Fn>
	void ForEach(Fn&& fn) const
	{
		for (const auto& n : nodes_)
		{
			if (n != nullptr)
			{
				fn(*n);
			}
		}
	}

	/**
	 * @brief Create field canvas (once) and init each node's visuals.
	 */
	void InitAllVisuals(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 fieldOrigin);

	/**
	 * @brief Move field origin for nodes and background canvas together.
	 */
	void SetFieldOrigin(DirectX::XMFLOAT3 fieldOrigin) noexcept;

	void SyncAllVisuals();

	/** @brief Submit field canvas only (call before any zone's nodes). */
	void SubmitBackground();

	/** @brief Submit loadout nodes only. */
	void SubmitNodes();

	/** @brief Convenience: SubmitBackground then SubmitNodes (not for cross-zone ordering). */
	void SubmitAllVisuals();

	/**
	 * @brief True if circle in world space lies entirely inside the field square.
	 */
	[[nodiscard]] bool ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept;

	/**
	 * @brief Clamp a field-local point so a circle of @p hitRadius stays inside kHalfExtent.
	 */
	[[nodiscard]] DirectX::XMFLOAT2 ClampLocalForRadius(
		DirectX::XMFLOAT2 localPos,
		float hitRadius) const noexcept;

	/**
	 * @brief True if a circle at @p fieldLocal would overlap any other node (excluding @p self).
	 */
	[[nodiscard]] bool WouldOverlap(
		const IFieldNode& self,
		DirectX::XMFLOAT2 fieldLocal) const noexcept;

	/** @brief End layout ghost on every owned node. */
	void ClearLayoutGhosts()
	{
		ForEach([](IFieldNode& node)
		{
			if (node.IsLayoutGhostActive())
			{
				node.EndLayoutGhost();
			}
		});
	}

	/**
	 * @brief Nearest node whose hit circle contains @p worldPos.
	 * @param outDistSq Distance squared from @p worldPos to the hit node center when found.
	 */
	[[nodiscard]] IFieldNode* PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept;

private:
	std::vector<std::unique_ptr<IFieldNode>> nodes_;
	std::unique_ptr<ModuleFieldCanvas> canvas_;
	DirectX::XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
};
