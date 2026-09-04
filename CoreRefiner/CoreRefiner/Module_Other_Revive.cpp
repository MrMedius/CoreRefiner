#include "Module_Other_Revive.h"
#include "AttackDeployer.h"
#include "XMath.h"

void Module_Other_Revive::OnSpawn()
{
	armed_ = true;
	if (Attack* owner = GetOwner())
	{
		launchDir_ = owner->GetMoveAccel();
	}
}

void Module_Other_Revive::OnDisable()
{
	Attack* owner = GetOwner();
	if (owner == nullptr || !armed_ || owner->IsActive())
	{
		return;
	}

	Replay_(owner);
}

void Module_Other_Revive::OnRecycle()
{
	armed_ = false;
	launchDir_ = { 0.0f, 0.0f, 0.0f };
}

void Module_Other_Revive::Replay_(Attack* owner)
{
	if (owner == nullptr || gfx_ == nullptr || rg_ == nullptr)
	{
		return;
	}

	const std::vector<AttackStepRecord>& records = owner->GetAssembledRecipe();
	if (records.empty())
	{
		return;
	}

	const DirectX::XMFLOAT3 pos = owner->GetWorldPosition();
	DirectX::XMFLOAT3 dir = launchDir_;
	if (dir.x == 0.0f && dir.y == 0.0f && dir.z == 0.0f && player_ != nullptr && player_->IsActive())
	{
		dir = (V(pos) - V(player_->GetWorldPosition())).ToFloat3();
		if (NormalizeXZ(dir))
		{
			dir = (V(dir) * AttackManager::kAimSpeed).ToFloat3();
		}
	}

	DeployContext ctx{};
	ctx.standby.gfx = gfx_;
	ctx.standby.rg = rg_;
	ctx.standby.spawnPos = pos;
	ctx.standby.player = player_;

	/**
	 * @brief 消耗封存列表里第一条 Other_Revive：只把 Focus 拨回主体，不 Apply、不记账、不进入 recordOnly。
	 * @note 其后的 Revive 仍走 ApplyAttackStepRecord（挂一层模块）。Flush 封到新根的是剩余配方。
	 */
	bool consumedRevive = false;
	for (const AttackStepRecord& rec : records)
	{
		if (!consumedRevive && rec.label == ModuleNodeLabel::Other_Revive)
		{
			consumedRevive = true;
			if (ctx.standby.parent != nullptr)
			{
				ctx.standby.host = ctx.standby.parent;
			}
			continue;
		}
		ApplyAttackStepRecord(ctx, rec);
	}
	ctx.FlushStandby();

	for (Attack* root : ctx.shots)
	{
		if (root == nullptr)
		{
			continue;
		}
		root->SetLaunchPosLocked(true);
		owner->AdoptLive(root);
		root->SpawnAt(pos, dir);
	}
}
