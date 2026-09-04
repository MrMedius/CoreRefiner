#pragma once

#include "AttackContext.h"
#include "ModuleNodeLabel.h"

class IAttackNodeStep
{
public:
	virtual ~IAttackNodeStep() = default;
	virtual void Apply(DeployContext& ctx) = 0;
	[[nodiscard]] virtual ModuleNodeLabel GetModuleNodeLabel() const noexcept = 0;
	[[nodiscard]] virtual const char* GetName() const noexcept = 0;
	[[nodiscard]] virtual bool HasModule() const noexcept { return false; }
	[[nodiscard]] virtual DeployTarget GetTarget() const noexcept { return DeployTarget::Focus; }

protected:
	IAttackNodeStep() = default;
};
