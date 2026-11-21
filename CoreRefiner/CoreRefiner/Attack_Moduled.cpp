#include "Attack_Moduled.h"

#include <vector>

void Attack_Moduled::OnCollide(Character* other)
{
	ApplyOwnHit(other);
	DispatchOnHit(other);
}

void Attack_Moduled::RequestDisable()
{
	ReleaseLivingChildren_();
	ObjectBase::RequestDisable();
}

void Attack_Moduled::Deactivate()
{
	DispatchOnDisable();
	ObjectBase::Deactivate();
	ClearModules();
}

void Attack_Moduled::DetachSelfFromParent_()
{
	ObjectBase* parent = GetParent();
	if (parent == nullptr)
	{
		return;
	}

	const XMFLOAT3 world = GetWorldPosition();
	if (auto* parentAtk = dynamic_cast<Attack_Moduled*>(parent))
	{
		ResetMoveVelocity();
		CalculateMoveVelocity(parentAtk->GetMoveVelocity());
		SetMoveAccel(parentAtk->GetMoveAccel());
		parentAtk->AdoptLive(this);
	}
	ClearParent();
	SetPosition(world);
	SetAwaitingManagerAdopt(true);
}

void Attack_Moduled::ReleaseLivingChildren_()
{
	std::vector<Attack_Moduled*> living;
	const std::size_t n = GetChildCount();
	living.reserve(n);
	for (std::size_t i = 0; i < n; ++i)
	{
		if (auto* child = dynamic_cast<Attack_Moduled*>(GetChild(i));
			child != nullptr && child->IsActive())
		{
			living.push_back(child);
		}
	}

	for (Attack_Moduled* child : living)
	{
		child->DetachSelfFromParent_();
	}
}
