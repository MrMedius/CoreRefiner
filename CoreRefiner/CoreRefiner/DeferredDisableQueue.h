#pragma once
#include <vector>

class ObjectBase;

// Frame-end queue for deferred ObjectBase::Deactivate (GM31 SetDestroy equivalent).
// Call Flush once per frame after all gameplay Updates.
class DeferredDisableQueue
{
public:
	static DeferredDisableQueue& Get() noexcept;

	// Queue an object for Deactivate at Flush (idempotent).
	void Enqueue(ObjectBase* object);

	// Remove if present (Activate / immediate Deactivate / cancel).
	void Remove(ObjectBase* object) noexcept;

	// Deactivate all queued objects and clear the queue.
	void Flush();

private:
	DeferredDisableQueue() = default;

	std::vector<ObjectBase*> pending_;
};
