#pragma once

#include "IFieldNode.h"

#include <DirectXMath.h>
#include <memory>
#include <utility>
#include <vector>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

class ModuleField
{
public:
	static constexpr float kHalfExtent = 150.0f;

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
	 * @brief Init each node's self-owned canvases (call once after placing nodes).
	 */
	void InitAllVisuals(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 fieldOrigin)
	{
		for (auto& n : nodes_)
		{
			if (n != nullptr)
			{
				n->InitVisual(gfx, rg, fieldOrigin);
			}
		}
	}

	void SyncAllVisuals()
	{
		for (auto& n : nodes_)
		{
			if (n != nullptr)
			{
				n->SyncVisual();
			}
		}
	}

	void SubmitAllVisuals()
	{
		for (auto& n : nodes_)
		{
			if (n != nullptr)
			{
				n->SubmitVisual();
			}
		}
	}

private:
	std::vector<std::unique_ptr<IFieldNode>> nodes_;
};
