#include "Other_Revive_Module.h"
#include "AttackDeployer.h"
#include "XMath.h"

void Other_Revive_Module::OnSpawn()
{
	armed_ = true;
	if (Attack* owner = GetOwner())
	{
		launchDir_ = owner->GetMoveAccel();
	}
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
	launchDir_ = { 0.0f, 0.0f, 0.0f };
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
	DirectX::XMFLOAT3 dir = launchDir_;
	if (dir.x == 0.0f && dir.y == 0.0f && dir.z == 0.0f && player_ != nullptr && player_->IsActive())
	{
		dir = (V(pos) - V(player_->GetWorldPosition())).ToFloat3();
		if (NormalizeXZ(dir))
		{
			dir = (V(dir) * AttackManager::kAimSpeed).ToFloat3();
		}
	}

	const std::vector<Attack*> roots = AttackDeployer::DeployRecords(
		records,
		*gfx_,
		*rg_,
		pos,
		player_);

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
