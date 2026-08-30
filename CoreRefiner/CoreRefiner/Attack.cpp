#include "Attack.h"

#include <vector>

void Attack::RequestDisable()
{
	ReleaseLivingChildren_();
	ObjectBase::RequestDisable();
}

void Attack::Deactivate()
{
	ObjectBase::Deactivate();
	ClearModules();
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

	const XMFLOAT3 parentVel = GetMoveVelocity();
	const XMFLOAT3 parentAccel = GetMoveAccel();
	for (Attack* child : living)
	{
		const XMFLOAT3 world = child->GetWorldPosition();
		child->ResetMoveVelocity();
		child->CalculateMoveVelocity(parentVel);
		child->SetMoveAccel(parentAccel);
		child->ClearParent();
		child->SetPosition(world);
		child->SetAwaitingManagerAdopt(true);
	}
}
