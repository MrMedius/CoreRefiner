#include "Attack.h"

void Attack::Deactivate()
{
	ObjectBase::Deactivate();
	ClearModules();
}
