#pragma once

#include "AttackNodeSteps.h"

class AttackDeployer
{
public:
	// 按封存快照装配一棵树。不 SpawnAt，由调用方 AdoptLive 后开火。
	// Revive 世代消耗不在这里；由 Module_Rule_Revive::Replay_ 在 Apply 前跳过第一条。
	static std::vector<Attack*> DeployRecords(const std::vector<AttackStepRecord>& records, Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 pos, Player* player)
	{
		DeployContext ctx{};
		ctx.standby.gfx = &gfx;
		ctx.standby.rg = &rg;
		ctx.standby.spawnPos = pos;
		ctx.standby.player = player;

		for (const AttackStepRecord& rec : records)
		{
			ApplyAttackStepRecord(ctx, rec);
		}

		ctx.FlushStandby();
		return ctx.shots;
	}
};
