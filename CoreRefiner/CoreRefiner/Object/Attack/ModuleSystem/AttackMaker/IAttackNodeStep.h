#pragma once

#include "AttackContext.h"

// 装配步骤。只把一条记录做进当前组装现场，不提供查询。
class IAttackNodeStep
{
public:
	virtual ~IAttackNodeStep() = default;
	// 把这一步写进 ctx。可能生成弹、装模块，或只改栏位。
	virtual void Apply(DeployContext& ctx) = 0;

protected:
	IAttackNodeStep() = default;
};
