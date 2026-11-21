#include "AttackBase_Moduled.h"
#include "ObjectCodex.h"

#include <vector>

void AttackBase_Moduled::OnCollide(CharacterBase* other)
{
	ApplyOwnHit(other);
	DispatchOnHit(other);
}

void AttackBase_Moduled::RequestDisable()
{
	ReleaseLivingChildren_();
	ObjectBase::RequestDisable();
}

void AttackBase_Moduled::Deactivate()
{
	DispatchOnDisable();
	ObjectBase::Deactivate();
	ClearModules();
}

void AttackBase_Moduled::DetachSelfFromParent_()
{
	ObjectBase* parent = GetParent();
	if (parent == nullptr)
	{
		return;
	}

	const XMFLOAT3 world = GetWorldPosition();
	if (auto* parentAtk = dynamic_cast<AttackBase_Moduled*>(parent))
	{
		ResetMoveVelocity();
		CalculateMoveVelocity(parentAtk->GetMoveVelocity());
		SetMoveAccel(parentAtk->GetMoveAccel());
	}
	ClearParent();
	SetPosition(world);
	// 交给攻击管理器认领。这一帧的更新已经开始，下一帧才进名单。
	ObjectCodex::Handoff(attack_Ball, this);
}

void AttackBase_Moduled::ReleaseLivingChildren_()
{
	std::vector<AttackBase_Moduled*> living;
	const std::size_t n = GetChildCount();
	living.reserve(n);
	for (std::size_t i = 0; i < n; ++i)
	{
		if (auto* child = dynamic_cast<AttackBase_Moduled*>(GetChild(i));
			child != nullptr && child->IsActive())
		{
			living.push_back(child);
		}
	}

	for (AttackBase_Moduled* child : living)
	{
		child->DetachSelfFromParent_();
	}
}
