#pragma once

#include "FieldModuleNode.h"
#include "ModuleFieldDraw.h"
#include "Canvas2D.h"
#include "Colors.h"

#include <cmath>
#include <memory>
#include <utility>
#include <vector>

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

	[[nodiscard]] FieldModuleNode* GetCore() const noexcept
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

	[[nodiscard]] FieldModuleNode* GetNode(std::size_t index) const noexcept
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

	void Redraw(Canvas2D& bg) const
	{
		const Color bgColor(100u, 150u, 50u, 150u);
		bg.Clear(bgColor);

		const unsigned cw = bg.GetCanvasWidth();
		const unsigned ch = bg.GetCanvasHeight();

		for (const auto& n : nodes_)
		{
			if (n == nullptr)
			{
				continue;
			}

			int px = 0;
			int py = 0;
			ModuleFieldDraw::LocalToPixel(
				n->GetLocalPos().x, n->GetLocalPos().y, cw, ch, px, py);

			const int r = static_cast<int>(std::lround(n->GetHitRadius()));
			if (n->IsCore())
			{
				const Color fill = n->IsReady()
					? Color(255u, 210u, 60u, 255u)
					: Color(120u, 100u, 40u, 255u);
				ModuleFieldDraw::DrawDisk(bg, px, py, r, fill);
				ModuleFieldDraw::DrawRing(bg, px, py, r + 2, 2, Color(255u, 240u, 160u, 255u));
			}
			else if (n->IsReady())
			{
				ModuleFieldDraw::DrawDisk(bg, px, py, r, Color(120u, 200u, 255u, 255u));
			}
			else
			{
				ModuleFieldDraw::DrawDisk(bg, px, py, r, Color(55u, 55u, 60u, 255u));
				ModuleFieldDraw::DrawRing(bg, px, py, r, 2, Color(100u, 100u, 110u, 200u));
			}
		}

		bg.NotifyPixelsChanged();
	}

private:
	std::vector<std::unique_ptr<FieldModuleNode>> nodes_;
};
