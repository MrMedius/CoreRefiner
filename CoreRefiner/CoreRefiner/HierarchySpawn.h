#pragma once
#include "ObjectCodex.h"

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

/**
 * @brief Assemble parent/child ObjectBase graphs via ObjectCodex pools (no pool-algorithm change).
 * @note SpawnPooled reuses inactive instances; this layer only ClearParent / SetParent / setup.
 *       Parent RequestDisable/Deactivate already cascades children and clears links (ObjectBase).
 */
namespace HierarchySpawn
{
	/**
	 * @brief Non-owning pointers into ObjectCodex pools after AssembleParentChildren.
	 */
	template <typename ParentT, typename ChildT>
	struct ParentChildren
	{
		ParentT* parent{ nullptr };
		std::vector<ChildT*> children;
	};

	/**
	 * @brief Warm up a tag pool: SpawnPooled then Deactivate (same pattern as AttackManager balls).
	 */
	template <typename T, typename... Args>
	void WarmupPool(Object_Type_Tag tag, int count, Args&&... args)
	{
		static_assert(std::is_base_of_v<ObjectBase, T>, "T must inherit from ObjectBase");
		std::vector<T*> batch;
		batch.reserve(static_cast<std::size_t>(count));
		for (int i = 0; i < count; ++i)
		{
			if (T* o = ObjectCodex::SpawnPooled<T>(tag, args...))
			{
				batch.push_back(o);
			}
		}
		for (T* o : batch)
		{
			o->Deactivate();
		}
	}

	/**
	 * @brief Spawn/reuse one parent and N children, wire hierarchy, then run setupChild.
	 * @tparam SetupChildFn void(ChildT* child, std::size_t index) — local pose / gameplay setup.
	 * @param parentTag / childTag Must differ so pools do not mix.
	 * @param childCtorArgs Args pack applied to every child SpawnPooled (refs OK).
	 * @param parentArgs Forwarded to SpawnPooled&lt;ParentT&gt;.
	 * @note ClearParent on both sides before SetParent to scrub pooled dirty links.
	 */
	template <
		typename ParentT,
		typename ChildT,
		typename SetupChildFn,
		typename... ParentArgs,
		typename... ChildArgs>
	ParentChildren<ParentT, ChildT> AssembleParentChildren(
		Object_Type_Tag parentTag,
		Object_Type_Tag childTag,
		std::size_t childCount,
		SetupChildFn&& setupChild,
		const std::tuple<ChildArgs...>& childCtorArgs,
		ParentArgs&&... parentArgs)
	{
		static_assert(std::is_base_of_v<ObjectBase, ParentT>, "ParentT must inherit from ObjectBase");
		static_assert(std::is_base_of_v<ObjectBase, ChildT>, "ChildT must inherit from ObjectBase");

		ParentChildren<ParentT, ChildT> out;
		out.parent = ObjectCodex::SpawnPooled<ParentT>(
			parentTag, std::forward<ParentArgs>(parentArgs)...);
		if (out.parent == nullptr)
		{
			return out;
		}

		out.parent->ClearParent();
		out.children.reserve(childCount);

		for (std::size_t i = 0; i < childCount; ++i)
		{
			ChildT* child = std::apply(
				[childTag](auto&&... args) -> ChildT*
				{
					return ObjectCodex::SpawnPooled<ChildT>(
						childTag, std::forward<decltype(args)>(args)...);
				},
				childCtorArgs);

			if (child == nullptr)
			{
				continue;
			}

			child->ClearParent();
			child->SetParent(out.parent);
			setupChild(child, i);
			out.children.push_back(child);
		}

		return out;
	}

	/**
	 * @brief Disable hierarchy root (cascades children via ObjectBase).
	 */
	inline void RequestDisableRoot(ObjectBase* root) noexcept
	{
		if (root == nullptr || !root->IsActive())
		{
			return;
		}
		root->RequestDisable();
	}
}
