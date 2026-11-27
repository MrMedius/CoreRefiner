#include "DeferredDisableQueue.h"
#include "ObjectBase.h"

#include <algorithm>

DeferredDisableQueue& DeferredDisableQueue::Get() noexcept
{
	static DeferredDisableQueue instance;
	return instance;
}

void DeferredDisableQueue::Enqueue(ObjectBase* object)
{
	if (object == nullptr)
	{
		return;
	}
	if (std::find(pending_.begin(), pending_.end(), object) != pending_.end())
	{
		return;
	}
	pending_.push_back(object);
}

void DeferredDisableQueue::Remove(ObjectBase* object) noexcept
{
	if (object == nullptr)
	{
		return;
	}
	pending_.erase(std::remove(pending_.begin(), pending_.end(), object), pending_.end());
}

void DeferredDisableQueue::Flush()
{
	std::vector<ObjectBase*> toDisable = std::move(pending_);
	pending_.clear();
	for (ObjectBase* object : toDisable)
	{
		if (object != nullptr)
		{
			object->Deactivate();
		}
	}
}
