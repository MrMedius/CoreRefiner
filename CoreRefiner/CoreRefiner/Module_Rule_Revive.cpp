#include "Module_Rule_Revive.h"
#include "AttackBase_Moduled.h"
#include "AttackDeployer.h"
#include "ObjectCodex.h"
#include "XMath.h"

void Module_Rule_Revive::OnSpawn()
{
	armed_ = true;
	if (AttackBase* owner = GetOwner())
	{
		launchDir_ = owner->GetMoveAccel();
	}
}

void Module_Rule_Revive::OnDisable()
{
	AttackBase* owner = GetOwner();
	if (owner == nullptr || !armed_ || owner->IsActive())
	{
		return;
	}

	Replay_(owner);
}

void Module_Rule_Revive::OnRecycle()
{
	armed_ = false;
	launchDir_ = { 0.0f, 0.0f, 0.0f };
}

void Module_Rule_Revive::Replay_(AttackBase* owner)
{
	if (owner == nullptr || gfx_ == nullptr || rg_ == nullptr)
	{
		return;
	}

	auto* moduled = dynamic_cast<AttackBase_Moduled*>(owner);
	if (moduled == nullptr)
	{
		return;
	}

	const std::vector<AttackStepRecord>& records = moduled->GetAssembledRecipe();
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

	// 消耗封存列表里第一条 Rule_Revive：跳过它，不改 host。
	// 子物体已经打开的栏位留着，后面的生成照旧填进去。
	// 其后的 Revive 仍走 ApplyAttackStepRecord（挂一层模块）。Flush 封到新根的是剩余配方。
	// consumedRevive 为真表示第一条复活已经跳过。
	bool consumedRevive = false;
	for (const AttackStepRecord& rec : records)
	{
		if (!consumedRevive && rec.label == ModuleNodeLabel::Rule_Revive)
		{
			consumedRevive = true;
			continue;
		}
		ApplyAttackStepRecord(ctx, rec);
	}
	ctx.FlushStandby();

	for (AttackBase* root : ctx.shots)
	{
		if (root == nullptr)
		{
			continue;
		}
		auto* rootMod = dynamic_cast<AttackBase_Moduled*>(root);
		if (rootMod != nullptr)
		{
			rootMod->SetLaunchPosLocked(true);
		}
		root->SpawnAt(pos, dir);
		// 交给攻击管理器认领。复活发生在这一帧的更新里，下一帧才进名单。
		ObjectCodex::Handoff(attack_Ball, root);
		if (rootMod != nullptr)
		{
			// 同伴排在主体后面。下一帧先更新主体，再让公转读它的位置。
			for (AttackBase_Moduled* child : rootMod->TakeBornWith())
			{
				if (child != nullptr)
				{
					ObjectCodex::Handoff(attack_Ball, child);
				}
			}
		}
	}
}
