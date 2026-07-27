#pragma once
#include "IComponent.h"
#include "Drawable.h"
#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

/**
 * @brief Owns a Drawable, syncs host transform into it, and submits to the RenderGraph.
 */
class VisualComponent : public IComponent
{
public:
	/**
	 * @param owner Host entity (injected by AddComponent).
	 * @param drawable Owned visual mesh/skin.
	 * @param channelMask RenderGraph channel bits (e.g. Chan::main | Chan::shadow).
	 * @param yawOnly If true, only yaw is written to the drawable (Enemy_T style).
	 * @param syncScale If true, also copy host scale each sync.
	 */
	VisualComponent(
		ObjectBase* owner,
		std::unique_ptr<Drawable> drawable,
		std::size_t channelMask,
		bool yawOnly = false,
		bool syncScale = false) noexcept;

	~VisualComponent() override;

	void OnEnable() override;
	void Update(float dt) override;
	void Submit() override;

	/** @brief Non-owning access to the owned Drawable. */
	[[nodiscard]] Drawable* GetDrawable() noexcept { return drawable_.get(); }
	[[nodiscard]] const Drawable* GetDrawable() const noexcept { return drawable_.get(); }

	/**
	 * @brief Typed access to the owned Drawable.
	 * @tparam T Must derive from Drawable.
	 */
	template <typename T>
	[[nodiscard]] T* GetDrawableAs() noexcept
	{
		static_assert(std::is_base_of_v<Drawable, T>, "T must inherit from Drawable");
		return dynamic_cast<T*>(drawable_.get());
	}
	template <typename T>
	[[nodiscard]] const T* GetDrawableAs() const noexcept
	{
		static_assert(std::is_base_of_v<Drawable, T>, "T must inherit from Drawable");
		return dynamic_cast<const T*>(drawable_.get());
	}

private:
	/** @brief Copy host transform fields into the drawable. */
	void SyncFromOwner();

	std::unique_ptr<Drawable> drawable_;
	std::size_t channelMask_{ 0 };
	bool yawOnly_{ false };
	bool syncScale_{ false };
};
