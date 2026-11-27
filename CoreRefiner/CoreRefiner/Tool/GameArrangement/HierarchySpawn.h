#pragma once
#include "ObjectCodex.h"

#include <cstddef>
#include <type_traits>
#include <vector>

// 对象池预热。按标签造好再立刻关掉，不交接，所以不会进管理器名单。
namespace HierarchySpawn
{
	// tag 是对象池标签。count 是要预热的个数。args 是传给 SpawnPooled 的构造参数。
	template <typename T, typename... Args>
	void WarmupPool(Object_Type_Tag tag, int count, Args&&... args)
	{
		static_assert(std::is_base_of_v<ObjectBase, T>, "T must inherit from ObjectBase");
		// batch 先收齐再关闭，避免边造边关时把刚造的又拿出来。
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
}
