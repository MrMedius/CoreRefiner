#pragma once
#include "IProjectileModule.h"
#include "Attack.h"

class Graphics;
class Player;

namespace Rgph
{
	class RenderGraph;
}

/**
 * @brief 只挂在根弹上：第一次只装配本节点之前的树；死亡时按封存配方再 Deploy（不含本节点，含之后的 Step）。
 * @note Reset / 池回收走直接 Deactivate（owner 仍 Active）时不放弹。
 */
class Other_Revive_Module : public IProjectileModule
{
public:
	Other_Revive_Module(
		Attack* owner,
		Graphics* gfx,
		Rgph::RenderGraph* rg,
		Player* player) noexcept
		:
		IProjectileModule(owner),
		gfx_(gfx),
		rg_(rg),
		player_(player)
	{}

	[[nodiscard]] bool HasModuleNodeLabel() const noexcept override { return true; }
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Other_Revive;
	}

	void OnSpawn() override;
	void OnDisable() override;
	void OnRecycle() override;

private:
	/** @brief 在消失点按配方装配新树并 AdoptLive + SpawnAt；Revive 只拨回 Focus，发射方向用 kAimSpeed。 */
	void Replay_(Attack* owner);

	Graphics* gfx_{ nullptr };
	Rgph::RenderGraph* rg_{ nullptr };
	Player* player_{ nullptr };
	bool armed_{ false };
};
