#include "AttackBase_Moduled.h"

#include <vector>

void AttackBase_Moduled::OnCollide(CharacterBase* other)
{
	// 还没提交过绘制就不结算。普通伤害和模块的 OnHit 都不跑。
	if (!IsPresentedForHit_())
	{
		return;
	}
	ApplyOwnHit(other);
	DispatchOnHit(other);
}

void AttackBase_Moduled::Deactivate()
{
	DispatchOnDisable();
	const std::vector<AttackBase_Moduled*> pending = bornWith_;
	bornWith_.clear();
	// 还没一起出生的子弹跟着关掉。已经交出的不在这份名单里，不会跟着消失。
	for (AttackBase_Moduled* child : pending)
	{
		if (child != nullptr && child->IsActive())
		{
			child->Deactivate();
		}
	}
	ObjectBase::Deactivate();
	ClearModules();
}
