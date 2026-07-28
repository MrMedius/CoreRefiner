#pragma once
#include <vector>

class ObjectBase;

/**
 * @brief Frame-end queue for deferred ObjectBase::Deactivate (GM31 SetDestroy equivalent).
 * @note Call Flush once per frame after all gameplay Updates.
 */
class DeferredDisableQueue
{
public:
	static DeferredDisableQueue& Get() noexcept;

	/** @brief Queue an object for Deactivate at Flush (idempotent). */
	void Enqueue(ObjectBase* object);

	/** @brief Remove if present (Activate / immediate Deactivate / cancel). */
	void Remove(ObjectBase* object) noexcept;

	/** @brief Deactivate all queued objects and clear the queue. */
	void Flush();

private:
	DeferredDisableQueue() = default;

	std::vector<ObjectBase*> pending_;
};
