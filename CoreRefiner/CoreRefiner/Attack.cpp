#include "Attack.h"

#include <vector>

void Attack::RequestDisable()
{
	ReleaseLivingChildren_();
	ObjectBase::RequestDisable();
}

void Attack::Deactivate()
{
	DispatchOnDisable();
	ObjectBase::Deactivate();
	ClearModules();
}

void Attack::DetachSelfFromParent_()
{
	ObjectBase* parent = GetParent();
	if (parent == nullptr)
	{
		return;
	}

	const XMFLOAT3 world = GetWorldPosition();
	if (auto* parentAtk = dynamic_cast<Attack*>(parent))
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

void Attack::ReleaseLivingChildren_()
{
	std::vector<Attack*> living;
	const std::size_t n = GetChildCount();
	living.reserve(n);
	for (std::size_t i = 0; i < n; ++i)
	{
		if (auto* child = dynamic_cast<Attack*>(GetChild(i));
			child != nullptr && child->IsActive())
		{
			living.push_back(child);
		}
	}

	for (Attack* child : living)
	{
		child->DetachSelfFromParent_();
	}
}
