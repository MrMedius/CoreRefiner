#pragma once
#include "IModule.h"
#include "Attack.h"

class Graphics;
class Player;

namespace Rgph
{
	class RenderGraph;
}

// 只挂在根弹上：第一次只装配本节点之前的树；死亡时消耗封存配方里第一条 Revive，再 Deploy 剩余列表。
// 剩余列表里若还有 Revive，新根再挂一层（同一发仍最多一层）。Reset / 池回收（owner 仍 Active）时不放弹。
class Module_Rule_Revive : public IModule
{
public:
	Module_Rule_Revive(
		Attack* owner,
		Graphics* gfx,
		Rgph::RenderGraph* rg,
		Player* player) noexcept
		:
		IModule(owner),
		gfx_(gfx),
		rg_(rg),
		player_(player)
	{}

	[[nodiscard]] bool HasModuleNodeLabel() const noexcept override { return true; }
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Rule_Revive;
	}

	void OnSpawn() override;
	void OnDisable() override;
	void OnRecycle() override;

private:
	// 在消失点按配方再 Deploy（消耗第一条 Revive 后封存剩余）；发射加速度用 OnSpawn 记下的那份，不跟鼠标。
	void Replay_(Attack* owner);

	Graphics* gfx_{ nullptr };
	Rgph::RenderGraph* rg_{ nullptr };
	Player* player_{ nullptr };
	// 第一次 SpawnAt/ArmModules 时的发射加速度（尚未乘 speed.Final()）。
	DirectX::XMFLOAT3 launchDir_{ 0.0f, 0.0f, 0.0f };
	bool armed_{ false };
};
