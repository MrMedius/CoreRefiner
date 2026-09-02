#include "Other_Revive_Module.h"
#include "AttackDeployer.h"
#include "XMath.h"

void Other_Revive_Module::OnSpawn()
{
	armed_ = true;
}

void Other_Revive_Module::OnDisable()
{
	Attack* owner = GetOwner();
	if (owner == nullptr || !armed_ || owner->IsActive())
	{
		return;
	}

	Replay_(owner);
}

void Other_Revive_Module::OnRecycle()
{
	armed_ = false;
}

void Other_Revive_Module::Replay_(Attack* owner)
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
	DirectX::XMFLOAT3 dir = owner->GetMoveAccel();
	if (dir.x == 0.0f && dir.y == 0.0f && dir.z == 0.0f)
	{
		dir = owner->GetMoveVelocity();
	}
	if (NormalizeXZ(dir))
	{
		dir = (V(dir) * AttackManager::kAimSpeed).ToFloat3();
	}
	else
	{
		dir = { 0.0f, 0.0f, 0.0f };
	}

	/** 本弹已 pendingDisable，池会当成可复用；重放期间先占住，避免 SpawnPooled 拿到自己。 */
	owner->SetReviveParked(true);
	const std::vector<Attack*> roots = AttackDeployer::DeployRecords(
		records,
		*gfx_,
		*rg_,
		pos,
		player_);
	owner->SetReviveParked(false);

	for (Attack* root : roots)
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
