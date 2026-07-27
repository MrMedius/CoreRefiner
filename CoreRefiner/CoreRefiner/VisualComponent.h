#pragma once
#include "IComponent.h"
#include <cstddef>

class Drawable;

/**
 * @brief Syncs host ObjectBase transform into a non-owning Drawable and submits it.
 */
class VisualComponent : public IComponent
{
public:
	/**
	 * @param owner Host entity (injected by AddComponent).
	 * @param drawable Non-owning visual; lifetime owned by the host.
	 * @param channelMask RenderGraph channel bits (e.g. Chan::main | Chan::shadow).
	 * @param yawOnly If true, only yaw is written to the drawable (Enemy_T style).
	 * @param syncScale If true, also copy host scale each sync.
	 */
	VisualComponent(
		ObjectBase* owner,
		Drawable* drawable,
		std::size_t channelMask,
		bool yawOnly = false,
		bool syncScale = false) noexcept;

	void OnEnable() override;
	void Update(float dt) override;
	void Submit() override;

private:
	/** @brief Copy host transform fields into the drawable. */
	void SyncFromOwner();

	Drawable* drawable_{ nullptr };
	std::size_t channelMask_{ 0 };
	bool yawOnly_{ false };
	bool syncScale_{ false };
};
